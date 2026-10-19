use super::*;
use qcm_core::crypto::{Hasher, MessageDigest};
use std::sync::mpsc::{Receiver, RecvTimeoutError};
use std::time::Duration;

pub const LEASE_SECONDS: u64 = 120;
const MAX_LEASES: i64 = 256;

pub struct CommitOutcome {
    pub complete: bool,
    pub newly_complete: bool,
}
pub struct LocalFile {
    pub path: PathBuf,
    pub generation: String,
    pub token: String,
    pub lifetime: u64,
}
#[derive(Debug, thiserror::Error)]
#[error("published cache file is corrupt")]
pub struct PlainCorrupt;

pub(super) struct Build {
    id: i64,
    pub(super) remaining: u64,
}
struct Ticket {
    version: Version,
    file: File,
}
pub(super) struct Threads {
    pub storage: std::sync::Mutex<Option<std::thread::JoinHandle<()>>>,
    worker: std::sync::Mutex<Option<std::thread::JoinHandle<()>>>,
    stop: Arc<AtomicBool>,
}
impl Threads {
    pub fn start(
        storage: std::thread::JoinHandle<()>,
        tx: mpsc::Sender<Job>,
        wake: Receiver<()>,
    ) -> Result<Self> {
        let stop = Arc::new(AtomicBool::new(false));
        let cancelled = stop.clone();
        let client = MaterializeClient { tx };
        let worker = std::thread::Builder::new()
            .name("resource-file".into())
            .spawn(move || {
                while !cancelled.load(Ordering::Acquire) {
                    match client.begin() {
                        Ok(Some(mut ticket)) => {
                            let version = ticket.version.clone();
                            let result = copy_blocks(&client, &mut ticket, &cancelled);
                            drop(ticket.file);
                            let published = match result {
                                Ok(digest) if !cancelled.load(Ordering::Acquire) => {
                                    client.publish(version.clone(), digest)
                                }
                                Ok(_) => Err(anyhow::anyhow!("file conversion cancelled")),
                                Err(err) => Err(err),
                            };
                            if let Err(err) = published {
                                log::warn!("cache file conversion deferred: {err}");
                                if let Err(err) = client.failed(version.id) {
                                    log::warn!("conversion cleanup deferred: {err}");
                                }
                            }
                        }
                        Ok(None) => match wake.recv_timeout(Duration::from_secs(1)) {
                            Ok(()) => {
                                let _ = client.rescan();
                            }
                            Err(RecvTimeoutError::Disconnected) => break,
                            Err(RecvTimeoutError::Timeout) => (),
                        },
                        Err(err) => {
                            log::warn!("cache file scheduler deferred: {err}");
                            if wake.recv_timeout(Duration::from_secs(1))
                                == Err(RecvTimeoutError::Disconnected)
                            {
                                break;
                            }
                        }
                    }
                }
            })?;
        Ok(Self {
            storage: std::sync::Mutex::new(Some(storage)),
            worker: std::sync::Mutex::new(Some(worker)),
            stop,
        })
    }
    pub fn stop_worker(&self) -> Result<()> {
        self.stop.store(true, Ordering::Release);
        if let Some(worker) = self.worker.lock().unwrap().take() {
            worker
                .join()
                .map_err(|_| anyhow::anyhow!("file worker panicked"))?;
        }
        Ok(())
    }
}
impl Drop for Threads {
    fn drop(&mut self) {
        let _ = self.stop_worker();
        if let Some(storage) = self.storage.get_mut().unwrap().take() {
            let _ = storage.join();
        }
    }
}
fn blocking_call<T: Send + 'static>(
    tx: &mpsc::Sender<Job>,
    f: impl FnOnce(&mut Storage) -> Result<T> + Send + 'static,
) -> Result<T> {
    let (reply, rx) = oneshot::channel();
    tx.blocking_send(Box::new(move |s| {
        let _ = reply.send(f(s));
    }))
    .map_err(|_| anyhow::anyhow!("cache storage stopped"))?;
    rx.blocking_recv()?
}
struct MaterializeClient {
    tx: mpsc::Sender<Job>,
}
impl MaterializeClient {
    fn begin(&self) -> Result<Option<Ticket>> {
        blocking_call(&self.tx, Storage::next_build)
    }
    fn read_block(&self, id: i64, block: u64) -> Result<Bytes> {
        blocking_call(&self.tx, move |s| {
            s.check_build(id)?;
            s.read(id, block)?
                .ok_or_else(|| anyhow::anyhow!("conversion block missing"))
        })
    }
    fn copied(&self, id: i64, bytes: u64) -> Result<()> {
        blocking_call(&self.tx, move |s| s.copied_block(id, bytes))
    }
    fn publish(&self, version: Version, digest: [u8; 32]) -> Result<()> {
        blocking_call(&self.tx, move |s| s.publish_file(version, digest))
    }
    fn failed(&self, id: i64) -> Result<()> {
        blocking_call(&self.tx, move |s| s.finish_failed(id))
    }
    fn rescan(&self) -> Result<()> {
        blocking_call(&self.tx, |s| {
            s.scan_cursor = 0;
            Ok(())
        })
    }
    #[cfg(test)]
    fn before_sync(&self) -> Result<()> {
        blocking_call(&self.tx, |s| s.fail_at(Fault::MaterializeSync))
    }
    #[cfg(test)]
    fn after_sync(&self) -> Result<()> {
        blocking_call(&self.tx, |s| s.crash_at(Fault::CrashMaterializeSync))
    }
}
fn copy_blocks(
    client: &MaterializeClient,
    ticket: &mut Ticket,
    stop: &AtomicBool,
) -> Result<[u8; 32]> {
    let mut hasher = Hasher::new(MessageDigest::sha256())?;
    let mut length = 0;
    for block in 0..ticket.version.meta.geometry.count() {
        ensure!(!stop.load(Ordering::Acquire), "file conversion cancelled");
        let id = ticket.version.id;
        let data = client.read_block(id, block)?;
        ticket.file.write_all(&data)?;
        hasher.update(&data)?;
        length += data.len() as u64;
        let bytes = allocated(&ticket.file.metadata()?);
        client.copied(id, bytes)?;
    }
    ensure!(
        length == ticket.version.meta.geometry.length,
        "conversion length mismatch"
    );
    #[cfg(test)]
    client.before_sync()?;
    ticket.file.sync_all()?;
    #[cfg(test)]
    client.after_sync()?;
    let digest: [u8; 32] = hasher.finish()?.as_ref().try_into()?;
    ticket.file.seek(SeekFrom::Start(0))?;
    let mut checked = Hasher::new(MessageDigest::sha256())?;
    let mut buffer = vec![0; ticket.version.meta.geometry.block.min(length) as usize];
    let mut verified = 0;
    loop {
        ensure!(!stop.load(Ordering::Acquire), "file verification cancelled");
        let count = ticket.file.read(&mut buffer)?;
        if count == 0 {
            break;
        }
        checked.update(&buffer[..count])?;
        verified += count as u64;
        ensure!(verified <= length, "file grew during verification");
    }
    ensure!(
        verified == length && checked.finish()?.as_ref() == digest,
        "file verification failed"
    );
    Ok(digest)
}

pub(super) fn private_directory(path: &Path) -> Result<()> {
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        fs::set_permissions(path, fs::Permissions::from_mode(0o700))?;
    }
    Ok(())
}
pub(super) fn private_file(file: &File) -> Result<()> {
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        file.set_permissions(fs::Permissions::from_mode(0o600))?;
    }
    Ok(())
}
pub(super) fn regular_file(path: &Path) -> Result<()> {
    ensure!(
        fs::symlink_metadata(path)?.file_type().is_file(),
        "invalid cache file type"
    );
    Ok(())
}
pub(super) fn managed_name(name: &str) -> bool {
    [".data", ".ok.tmp", ".ok"]
        .iter()
        .any(|suffix| name.strip_suffix(suffix).is_some_and(valid_generation))
}
fn ticket_reservation(length: u64) -> u64 {
    length.div_ceil(4096) * 4096
}

impl Store {
    pub async fn local_file(&self, version: Version, client: String) -> Result<Option<LocalFile>> {
        self.call_version(version, move |s, version| {
            s.acquire_file(version.id, &client)
        })
        .await
    }
    pub async fn renew_file(&self, token: String, client: String) -> Result<bool> {
        self.call(move |s| s.renew_file(&token, &client)).await
    }
    pub async fn release_file(&self, token: String, client: String) -> Result<()> {
        self.call(move |s| {
            s.db.statement(
                "DELETE FROM file_leases WHERE token=? AND client=?",
                &[Value::Text(&token), Value::Text(&client)],
            )?
            .next()?;
            s.evict(0)
        })
        .await
    }
    #[cfg(test)]
    pub async fn plain_ready(&self, version: Version) -> Result<bool> {
        self.call_version(version, move |s, version| s.plain(version.id))
            .await
    }
    #[cfg(test)]
    pub async fn test_materialization(&self, enabled: bool) -> Result<()> {
        self.call(move |s| {
            s.files_enabled = enabled;
            s.scan_cursor = 0;
            Ok(())
        })
        .await?;
        let _ = self.wake.try_send(());
        Ok(())
    }
    #[cfg(test)]
    pub async fn test_capacity(&self, capacity: u64) -> Result<()> {
        self.call(move |s| {
            s.capacity = capacity;
            Ok(())
        })
        .await
    }
    #[cfg(test)]
    pub async fn test_retry(&self, version: Version) -> Result<()> {
        self.call(move |s| {
            s.db.statement(
                "UPDATE complete_files SET retry_at=0 WHERE version_id=?",
                &[Value::Int(version.id)],
            )?
            .next()?;
            s.scan_cursor = 0;
            Ok(())
        })
        .await?;
        let _ = self.wake.try_send(());
        Ok(())
    }
    #[cfg(test)]
    pub async fn test_deferred(&self, version: Version) -> Result<bool> {
        self.call(move |s| {
            let mut row = s.db.statement(
                "SELECT retry_at FROM complete_files WHERE version_id=?",
                &[Value::Int(version.id)],
            )?;
            Ok(row.next()? && row.int(0) != 0)
        })
        .await
    }
    #[cfg(test)]
    pub async fn test_expire(&self, token: String) -> Result<()> {
        self.call(move |s| {
            s.db.statement(
                "UPDATE file_leases SET expires=0 WHERE token=?",
                &[Value::Text(&token)],
            )?
            .next()?;
            s.evict(0)
        })
        .await
    }
}

impl Storage {
    fn copied_block(&mut self, id: i64, bytes: u64) -> Result<()> {
        self.check_build(id)?;
        let v = self.load(id)?;
        let name = format!("{}.ok.tmp", v.meta.generation);
        let kind = v.meta.kind;
        let reserved = ticket_reservation(v.meta.geometry.length);
        self.sizes.insert(name, (bytes, kind));
        self.building.as_mut().unwrap().remaining = reserved.saturating_sub(bytes);
        #[cfg(test)]
        {
            self.fail_at(Fault::MaterializeWrite)?;
            self.crash_at(Fault::CrashMaterializeWrite)?;
        }
        Ok(())
    }
    pub(super) fn file_path(&self, generation: &str, suffix: &str) -> PathBuf {
        self.root.join(format!("{generation}.{suffix}"))
    }
    pub(super) fn plain(&self, id: i64) -> Result<bool> {
        Ok(self.plain_length(id)?.is_some())
    }
    pub(super) fn plain_length(&self, id: i64) -> Result<Option<u64>> {
        let mut row = self.db.statement(
            "SELECT ready,length,digest,format FROM complete_files WHERE version_id=?",
            &[Value::Int(id)],
        )?;
        if !row.next()? || row.int(0) != 1 {
            return Ok(None);
        }
        ensure!(
            row.int(1) >= 0 && row.blob(2).len() == 32 && row.int(3) == 1,
            "invalid plain descriptor"
        );
        Ok(Some(row.int(1) as u64))
    }
    fn next_build(&mut self) -> Result<Option<Ticket>> {
        if !self.files_enabled {
            return Ok(None);
        }
        self.expire_leases()?;
        self.cleanup_garbage();
        self.cleanup_plain_data()?;
        if let Err(err) = self.evict(0) {
            log::debug!("cache maintenance waiting: {err}");
        }
        let mut rows = self.db.statement("SELECT v.id FROM versions v LEFT JOIN complete_files f ON f.version_id=v.id WHERE v.obsolete=0 AND COALESCE(f.ready,0)=0 AND COALESCE(f.retry_at,0)<=? AND v.id>? ORDER BY v.id LIMIT 64", &[Value::Int(self.lease_now() as i64), Value::Int(self.scan_cursor)])?;
        let mut ids = Vec::new();
        while rows.next()? {
            ids.push(rows.int(0));
        }
        drop(rows);
        if ids.is_empty() {
            self.scan_cursor = 0;
            return Ok(None);
        }
        for id in ids {
            self.scan_cursor = id;
            let v = match self.load(id) {
                Ok(v) => v,
                Err(err) => {
                    log::warn!("file candidate unavailable: {err}");
                    continue;
                }
            };
            if v.meta.layout != Layout::Blocks
                || (v.meta.kind == 1 && v.meta.geometry.length > format::SEGMENT_BYTES)
                || !v.meta.policy.store
                || !v.index.is_committed()
                || !v.index.complete()
            {
                continue;
            }
            let version = self.version(id)?;
            let cleaned = (|| {
                for suffix in ["ok.tmp", "ok"] {
                    self.remove_managed(&format!("{}.{suffix}", version.meta.generation))?;
                }
                Ok::<_, anyhow::Error>(())
            })();
            if let Err(err) = cleaned {
                log::warn!("unpublished file cleanup deferred: {err}");
                self.defer_build(id)?;
                continue;
            }
            let reserved = ticket_reservation(version.meta.geometry.length);
            if let Err(err) = self.evict(reserved) {
                log::debug!("file conversion waiting for capacity: {err}");
                self.defer_build(id)?;
                continue;
            }
            let prepared = (|| {
                for suffix in ["ok.tmp", "ok"] {
                    self.remove_managed(&format!("{}.{suffix}", version.meta.generation))?;
                }
                let file = OpenOptions::new()
                    .write(true)
                    .read(true)
                    .create_new(true)
                    .open(self.file_path(&version.meta.generation, "ok.tmp"))?;
                private_file(&file)?;
                Ok::<_, anyhow::Error>(file)
            })();
            let file = match prepared {
                Ok(file) => file,
                Err(err) => {
                    log::warn!("cache file creation deferred: {err}");
                    self.defer_build(id)?;
                    continue;
                }
            };
            self.sizes.insert(
                format!("{}.ok.tmp", version.meta.generation),
                (allocated(&file.metadata()?), version.meta.kind),
            );
            self.building = Some(Build {
                id,
                remaining: reserved,
            });
            #[cfg(test)]
            self.crash_at(Fault::CrashMaterializeCreate)?;
            return Ok(Some(Ticket { version, file }));
        }
        Ok(None)
    }
    fn check_build(&self, id: i64) -> Result<()> {
        ensure!(
            self.building.as_ref().is_some_and(|b| b.id == id),
            "stale file ticket"
        );
        let mut row = self.db.statement(
            "SELECT obsolete FROM versions WHERE id=?",
            &[Value::Int(id)],
        )?;
        ensure!(row.next()? && row.int(0) == 0, "file generation retired");
        Ok(())
    }
    fn publish_file(&mut self, version: Version, digest: [u8; 32]) -> Result<()> {
        self.check_build(version.id)?;
        ensure!(version.is_valid(), "file provider invalidated");
        let v = self.load(version.id)?;
        ensure!(
            v.index.is_committed() && v.index.complete() && !v.plain,
            "file index not committed"
        );
        let temp = self.file_path(&version.meta.generation, "ok.tmp");
        let final_path = self.file_path(&version.meta.generation, "ok");
        regular_file(&temp)?;
        ensure!(
            fs::metadata(&temp)?.len() == version.meta.geometry.length,
            "file output size changed"
        );
        ensure!(!final_path.try_exists()?, "file already published");
        #[cfg(test)]
        self.fail_at(Fault::MaterializeRename)?;
        fs::rename(&temp, &final_path)?;
        self.sizes
            .remove(&format!("{}.ok.tmp", version.meta.generation));
        self.sizes.insert(
            format!("{}.ok", version.meta.generation),
            (allocated(&fs::metadata(&final_path)?), version.meta.kind),
        );
        if let Some(b) = &mut self.building {
            b.remaining = 0;
        }
        #[cfg(test)]
        self.crash_at(Fault::CrashMaterializeDirectory)?;
        sync_directory(&self.root)?;
        #[cfg(test)]
        self.crash_at(Fault::CrashMaterializeRename)?;
        let file = File::open(&final_path)?;
        self.db.exec("BEGIN IMMEDIATE")?;
        let result = (|| {
            #[cfg(test)]
            self.fail_at(Fault::MaterializeSql)?;
            self.db.statement("INSERT INTO complete_files(version_id,ready,length,digest,retry_at) VALUES(?,1,?,?,0) ON CONFLICT(version_id) DO UPDATE SET ready=1,length=excluded.length,digest=excluded.digest,retry_at=0", &[Value::Int(version.id), Value::Int(version.meta.geometry.length as i64), Value::Blob(&digest)])?.next()?;
            self.db.exec("COMMIT")
        })();
        if let Err(err) = result {
            let _ = self.db.exec("ROLLBACK");
            return Err(err);
        }
        #[cfg(test)]
        self.crash_at(Fault::CrashMaterializeSql)?;
        let v = self.versions.get_mut(&version.id).unwrap();
        v.file = Some(file);
        v.plain = true;
        self.building = None;
        if let Err(err) = self.cleanup_plain_data() {
            log::warn!("published file cleanup deferred: {err}");
        }
        Ok(())
    }
    fn defer_build(&mut self, id: i64) -> Result<()> {
        self.db.statement("INSERT INTO complete_files(version_id,retry_at) VALUES(?,?) ON CONFLICT(version_id) DO UPDATE SET retry_at=excluded.retry_at", &[Value::Int(id), Value::Int((self.lease_now() + 30) as i64)])?.next()?;
        Ok(())
    }
    fn finish_failed(&mut self, id: i64) -> Result<()> {
        self.building = None;
        self.defer_build(id)?;
        let v = self.load(id)?;
        let generation = v.meta.generation.clone();
        if !self.plain(id)? {
            for suffix in ["ok.tmp", "ok"] {
                let name = format!("{generation}.{suffix}");
                if let Ok(meta) = fs::symlink_metadata(self.root.join(&name)) {
                    let kind = self.versions[&id].meta.kind;
                    self.sizes.insert(name.clone(), (allocated(&meta), kind));
                }
                self.remove_managed(&name)?;
            }
        }
        Ok(())
    }
    fn cleanup_plain_data(&mut self) -> Result<()> {
        let mut rows = self.db.statement(
            "SELECT generation FROM versions JOIN complete_files ON version_id=id WHERE ready=1",
            &[],
        )?;
        let mut names = Vec::new();
        while rows.next()? {
            names.push(format!("{}.data", rows.text(0)?));
        }
        drop(rows);
        for name in names {
            if self.sizes.contains_key(&name) {
                #[cfg(test)]
                self.fail_at(Fault::MaterializeDelete)?;
                self.remove_managed(&name)?;
                #[cfg(test)]
                self.crash_at(Fault::CrashMaterializeDelete)?;
            }
        }
        Ok(())
    }
    pub(super) fn remove_managed(&mut self, name: &str) -> Result<()> {
        ensure!(
            managed_name(name) || segments::managed_name(name) || super::objects::managed_name(name),
            "unmanaged cache path"
        );
        let result = match fs::remove_file(self.root.join(name)) {
            Ok(()) => sync_directory(&self.root),
            Err(e) if e.kind() == std::io::ErrorKind::NotFound => sync_directory(&self.root),
            Err(e) => Err(e.into()),
        };
        if let Err(err) = result {
            self.garbage.insert(name.to_owned());
            return Err(err);
        }
        self.garbage.remove(name);
        self.sizes.remove(name);
        Ok(())
    }
    fn cleanup_garbage(&mut self) {
        let names: Vec<_> = self.garbage.iter().take(64).cloned().collect();
        for name in names {
            if let Err(err) = self.remove_managed(&name) {
                log::debug!("cache cleanup still deferred: {err}");
            }
        }
    }
    pub(super) fn retire_corrupt(&mut self, id: i64) -> Result<()> {
        if let Some(v) = self.versions.get(&id) {
            if let Some(lease) = v.lease.upgrade() {
                lease.valid.store(false, Ordering::Release);
            }
        }
        self.db
            .statement(
                "UPDATE versions SET obsolete=2 WHERE id=?",
                &[Value::Int(id)],
            )?
            .next()?;
        Ok(())
    }
    fn lease_now(&self) -> u64 {
        self.clock.saturating_add(self.started.elapsed().as_secs())
    }
    pub(super) fn restore_leases(&mut self) -> Result<()> {
        let mut row = self
            .db
            .statement("SELECT time FROM file_clock WHERE id=1", &[])?;
        let old = if row.next()? { row.int(0) as u64 } else { 0 };
        drop(row);
        self.clock_uncertain = self.clock < old;
        self.clock = self.clock.max(old);
        let mut row = self
            .db
            .statement("SELECT 1 FROM file_leases LIMIT 1", &[])?;
        if row.next()? {
            self.restart_grace =
                Some(std::time::Instant::now() + Duration::from_secs(LEASE_SECONDS));
        }
        drop(row);
        self.persist_clock()
    }
    fn persist_clock(&self) -> Result<()> {
        self.db.statement("INSERT INTO file_clock(id,time) VALUES(1,?) ON CONFLICT(id) DO UPDATE SET time=MAX(time,excluded.time)", &[Value::Int(self.lease_now() as i64)])?.next()?;
        Ok(())
    }
    fn expire_leases(&mut self) -> Result<()> {
        if super::super::policy::now() < self.clock {
            self.clock_uncertain = true;
        }
        if self.clock_uncertain && super::super::policy::now() >= self.lease_now() {
            self.clock_uncertain = false;
        }
        if !self.clock_uncertain
            && self
                .restart_grace
                .is_none_or(|until| std::time::Instant::now() >= until)
        {
            self.db
                .statement(
                    "DELETE FROM file_leases WHERE expires<=?",
                    &[Value::Int(self.lease_now() as i64)],
                )?
                .next()?;
        }
        Ok(())
    }
    pub(super) fn externally_pinned(&mut self, id: i64) -> Result<bool> {
        self.expire_leases()?;
        let mut row = self.db.statement(
            "SELECT 1 FROM file_leases WHERE version_id=? LIMIT 1",
            &[Value::Int(id)],
        )?;
        row.next()
    }
    fn acquire_file(&mut self, id: i64, client: &str) -> Result<Option<LocalFile>> {
        self.expire_leases()?;
        if self.clock_uncertain || !self.plain(id)? {
            return Ok(None);
        }
        let mut row = self.db.statement(
            "SELECT obsolete FROM versions WHERE id=?",
            &[Value::Int(id)],
        )?;
        if !row.next()? || row.int(0) != 0 {
            return Ok(None);
        }
        drop(row);
        let v = self.load(id)?;
        let generation = v.meta.generation.clone();
        let length = v.meta.geometry.length;
        if v.meta.kind != 0 {
            return Ok(None);
        }
        let path = self.file_path(&generation, "ok");
        if regular_file(&path).is_err() || fs::metadata(&path)?.len() != length {
            self.retire_corrupt(id)?;
            anyhow::bail!(PlainCorrupt);
        }
        let mut row = self.db.statement("SELECT COUNT(*) FROM file_leases", &[])?;
        row.next()?;
        if row.int(0) >= MAX_LEASES {
            return Ok(None);
        }
        drop(row);
        let mut row = self.db.statement("SELECT COALESCE(SUM(length),0) FROM complete_files WHERE version_id IN (SELECT DISTINCT version_id FROM file_leases WHERE version_id<>?)", &[Value::Int(id)])?;
        row.next()?;
        let protected = row.int(0) as u64;
        drop(row);
        if protected.saturating_add(length) > (self.capacity / 2).min(512 * 1024 * 1024) {
            return Ok(None);
        }
        let token = uuid::Uuid::new_v4().simple().to_string();
        self.persist_clock()?;
        self.db
            .statement(
                "INSERT INTO file_leases(token,version_id,client,expires) VALUES(?,?,?,?)",
                &[
                    Value::Text(&token),
                    Value::Int(id),
                    Value::Text(client),
                    Value::Int((self.lease_now() + LEASE_SECONDS) as i64),
                ],
            )?
            .next()?;
        Ok(Some(LocalFile {
            path,
            generation,
            token,
            lifetime: LEASE_SECONDS,
        }))
    }
    fn renew_file(&mut self, token: &str, client: &str) -> Result<bool> {
        self.expire_leases()?;
        if self.clock_uncertain {
            return Ok(false);
        }
        let mut row = self.db.statement("SELECT l.version_id FROM file_leases l JOIN versions v ON v.id=l.version_id WHERE token=? AND client=? AND obsolete=0 AND expires>?", &[Value::Text(token), Value::Text(client), Value::Int(self.lease_now() as i64)])?;
        if !row.next()? {
            return Ok(false);
        }
        let id = row.int(0);
        drop(row);
        let generation = self.load(id)?.meta.generation.clone();
        let path = self.file_path(&generation, "ok");
        if regular_file(&path).is_err()
            || fs::metadata(path)?.len() != self.versions[&id].meta.geometry.length
        {
            self.retire_corrupt(id)?;
            return Ok(false);
        }
        self.persist_clock()?;
        self.db
            .statement(
                "UPDATE file_leases SET expires=? WHERE token=? AND client=?",
                &[
                    Value::Int((self.lease_now() + LEASE_SECONDS) as i64),
                    Value::Text(token),
                    Value::Text(client),
                ],
            )?
            .next()?;
        Ok(true)
    }
    #[cfg(test)]
    pub(super) fn fail_at(&mut self, fault: Fault) -> Result<()> {
        if self.fault == Some(fault) {
            if fault != Fault::MaterializeDelete {
                self.fault = None;
            }
            anyhow::bail!("injected file failure {fault:?}");
        }
        Ok(())
    }
    #[cfg(test)]
    pub(super) fn crash_at(&self, fault: Fault) -> Result<()> {
        if self.fault == Some(fault) {
            std::process::exit(42);
        }
        Ok(())
    }
}
