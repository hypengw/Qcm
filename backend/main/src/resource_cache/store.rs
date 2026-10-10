use super::{
    format::{self, Geometry, Index, HEADER_LEN},
    policy::{Headers, Policy},
    sqlite::{Database, Value},
};
use anyhow::{ensure, Result};
use bytes::Bytes;
use serde::{Deserialize, Serialize};
use std::{
    collections::HashMap,
    fs::{self, File, OpenOptions},
    io::{Read, Seek, SeekFrom, Write},
    path::{Path, PathBuf},
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc, Weak,
    },
};
use tokio::sync::{mpsc, oneshot};

const CAPACITY: u64 = 32 * 1024 * 1024 * 1024;
const MAX_RESIDENT: usize = 128;
type Job = Box<dyn FnOnce(&mut Storage) + Send>;

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct Metadata {
    pub key: String,
    pub provider: i64,
    pub kind: i32,
    pub generation: String,
    pub geometry: Geometry,
    pub headers: Headers,
    pub policy: Policy,
    pub target: String,
    pub ranges: bool,
}
#[derive(Clone)]
pub struct Version {
    pub id: i64,
    pub meta: Metadata,
    pub(super) _lease: Arc<Lease>,
}
pub(super) struct Lease {
    valid: AtomicBool,
}
impl Version {
    pub fn is_valid(&self) -> bool {
        self._lease.valid.load(Ordering::Acquire)
    }
}
#[derive(Default, Debug)]
pub struct Usage {
    pub image: u64,
    pub audio: u64,
    pub metadata: u64,
}
struct Resident {
    meta: Metadata,
    index: Index,
    file: File,
    lease: Weak<Lease>,
}
struct Storage {
    db: Database,
    root: PathBuf,
    versions: HashMap<i64, Resident>,
    capacity: u64,
    stopped: bool,
    sizes: HashMap<String, (u64, i32)>,
    #[cfg(test)]
    fault: Option<Fault>,
}
#[cfg(test)]
#[derive(Clone, Copy, PartialEq)]
pub(super) enum Fault {
    Write,
    Sync,
    Sql,
    CrashSync,
    CrashSql,
}

#[derive(Clone)]
pub struct Store {
    tx: mpsc::Sender<Job>,
    thread: Arc<std::sync::Mutex<Option<std::thread::JoinHandle<()>>>>,
}
impl Store {
    pub async fn open(root: PathBuf) -> Result<Self> {
        let (tx, mut rx) = mpsc::channel::<Job>(8);
        let (ready_tx, ready_rx) = oneshot::channel();
        let thread = std::thread::Builder::new()
            .name("resource-cache".into())
            .spawn(move || {
                let opened = Storage::open(root);
                match opened {
                    Ok(mut storage) => {
                        let _ = ready_tx.send(Ok(()));
                        while let Some(job) = rx.blocking_recv() {
                            job(&mut storage);
                            if storage.stopped {
                                break;
                            }
                        }
                    }
                    Err(err) => {
                        let _ = ready_tx.send(Err(err));
                    }
                }
            })?;
        ready_rx.await??;
        Ok(Self {
            tx,
            thread: Arc::new(std::sync::Mutex::new(Some(thread))),
        })
    }
    async fn call<T: Send + 'static>(
        &self,
        f: impl FnOnce(&mut Storage) -> Result<T> + Send + 'static,
    ) -> Result<T> {
        let (tx, rx) = oneshot::channel();
        self.tx
            .send(Box::new(move |store| {
                let _ = tx.send(f(store));
            }))
            .await
            .map_err(|_| anyhow::anyhow!("cache storage stopped"))?;
        rx.await?
    }
    pub async fn lookup(&self, key: String) -> Result<Option<Version>> {
        self.call(move |s| s.lookup(&key)).await
    }
    pub async fn create(&self, meta: Metadata) -> Result<Version> {
        self.call(move |s| s.create(meta)).await
    }
    pub async fn read(&self, version: Version, block: u64) -> Result<Option<Bytes>> {
        self.call(move |s| s.read(version.id, block)).await
    }
    pub async fn write(&self, version: Version, blocks: Vec<(u64, Bytes)>) -> Result<()> {
        self.call(move |s| s.write(version.id, &blocks)).await
    }
    pub async fn complete(&self, version: Version) -> Result<bool> {
        self.call(move |s| Ok(s.load(version.id)?.index.complete()))
            .await
    }
    pub async fn covered(&self, version: Version, range: std::ops::Range<u64>) -> Result<bool> {
        self.call(move |s| {
            let v = s.load(version.id)?;
            Ok(
                (range.start / v.meta.geometry.block..range.end.div_ceil(v.meta.geometry.block))
                    .all(|i| v.index.get(i).is_some()),
            )
        })
        .await
    }
    pub async fn update(&self, version: Version, meta: Metadata) -> Result<()> {
        self.call(move |s| s.update(version.id, meta)).await
    }
    pub async fn missing_range(
        &self,
        version: Version,
        range: std::ops::Range<u64>,
    ) -> Result<std::ops::Range<u64>> {
        self.call(move |s| {
            let value = s.load(version.id)?;
            let block = value.meta.geometry.block;
            let mut end = range.end;
            for index in range.start / block + 1..range.end.div_ceil(block) {
                if value.index.get(index).is_some() {
                    end = index * block;
                    break;
                }
            }
            Ok(range.start..end)
        })
        .await
    }
    pub async fn invalidate(&self, provider: i64) -> Result<()> {
        self.call(move |s| s.invalidate(provider)).await
    }
    pub async fn usage(&self) -> Result<Usage> {
        self.call(Storage::usage).await
    }
    #[cfg(test)]
    pub async fn checkpoint(&self) -> Result<()> {
        self.call(|s| s.db.exec("PRAGMA wal_checkpoint(PASSIVE)"))
            .await
    }
    pub async fn retire(&self, version: Version) -> Result<()> {
        self.call(move |s| {
            s.db.statement(
                "UPDATE versions SET obsolete=1 WHERE id=?",
                &[Value::Int(version.id)],
            )?
            .next()?;
            Ok(())
        })
        .await
    }
    pub async fn shutdown(&self) -> Result<()> {
        self.call(|s| {
            s.db.exec("PRAGMA wal_checkpoint(PASSIVE)")?;
            s.stopped = true;
            Ok(())
        })
        .await?;
        let thread = self.thread.lock().unwrap().take();
        if let Some(thread) = thread {
            tokio::task::spawn_blocking(move || {
                thread
                    .join()
                    .map_err(|_| anyhow::anyhow!("storage thread panicked"))
            })
            .await??;
        }
        Ok(())
    }
    #[cfg(test)]
    pub(super) async fn fault(&self, fault: Fault) -> Result<()> {
        self.call(move |s| {
            s.fault = Some(fault);
            Ok(())
        })
        .await
    }
}

impl Storage {
    fn open(root: PathBuf) -> Result<Self> {
        fs::create_dir_all(&root)?;
        let db = Database::open(&root.join("meta.sqlite"))?;
        db.exec("CREATE TABLE IF NOT EXISTS versions (id INTEGER PRIMARY KEY, resource_key TEXT NOT NULL, provider INTEGER NOT NULL, kind INTEGER NOT NULL, generation TEXT NOT NULL UNIQUE, meta TEXT NOT NULL, blocks BLOB NOT NULL, last_use INTEGER NOT NULL, obsolete INTEGER NOT NULL DEFAULT 0); CREATE INDEX IF NOT EXISTS resource_lookup ON versions(resource_key, id);")?;
        let capacity = std::env::var("QCM_RESOURCE_CACHE_BYTES")
            .ok()
            .and_then(|s| s.parse().ok())
            .unwrap_or(CAPACITY);
        let mut store = Self {
            db,
            root,
            versions: HashMap::new(),
            capacity,
            stopped: false,
            sizes: HashMap::new(),
            #[cfg(test)]
            fault: None,
        };
        store.recover()?;
        Ok(store)
    }
    fn path(&self, generation: &str) -> PathBuf {
        self.root.join(format!("{generation}.data"))
    }
    fn recover(&mut self) -> Result<()> {
        let mut rows = self.db.statement("SELECT id, meta FROM versions", &[])?;
        let mut invalid = Vec::new();
        let mut retained = HashMap::new();
        while rows.next()? {
            let id = rows.int(0);
            match serde_json::from_str::<Metadata>(&rows.text(1)?) {
                Ok(meta) if valid_generation(&meta.generation) => {
                    retained.insert(format!("{}.data", meta.generation), meta.kind);
                }
                _ => {
                    invalid.push(id);
                }
            }
        }
        drop(rows);
        for id in invalid {
            self.delete(id)?;
        }
        for entry in fs::read_dir(&self.root)? {
            let entry = entry?;
            let name = entry.file_name().to_string_lossy().into_owned();
            if name.ends_with(".data")
                && valid_generation(name.trim_end_matches(".data"))
                && !retained.contains_key(&name)
            {
                fs::remove_file(entry.path())?;
            } else if let Some(kind) = retained.get(&name) {
                self.sizes.insert(
                    name.trim_end_matches(".data").into(),
                    (allocated(&entry.metadata()?), *kind),
                );
            }
        }
        self.evict(0)
    }
    fn load(&mut self, id: i64) -> Result<&mut Resident> {
        if !self.versions.contains_key(&id) {
            let mut row = self.db.statement(
                "SELECT meta, blocks FROM versions WHERE id=?",
                &[Value::Int(id)],
            )?;
            ensure!(row.next()?, "cache version removed");
            let meta: Metadata = serde_json::from_str(&row.text(0)?)?;
            ensure!(valid_generation(&meta.generation), "invalid generation");
            ensure!(
                matches!(
                    meta.geometry.block,
                    format::MIN_BLOCK | 524288 | format::MAX_BLOCK
                ),
                "invalid block size"
            );
            let bytes = row.blob(1);
            drop(row);
            let mut file = OpenOptions::new()
                .read(true)
                .write(true)
                .open(self.path(&meta.generation))?;
            let size = file.metadata()?.len();
            ensure!(size >= HEADER_LEN, "incomplete data header");
            let mut header = [0; HEADER_LEN as usize];
            file.read_exact(&mut header)?;
            ensure!(
                header == meta.geometry.header(&meta.generation)?,
                "data identity mismatch"
            );
            let slots = (size - HEADER_LEN) / meta.geometry.block;
            self.reserve_index(bytes.len())?;
            let index = Index::load(meta.geometry, bytes, slots)?;
            // A torn, unreferenced final slot can be discarded before the next append.
            file.set_len(HEADER_LEN + slots * meta.geometry.block)?;
            self.sizes.insert(
                meta.generation.clone(),
                (allocated(&file.metadata()?), meta.kind),
            );
            self.versions.insert(
                id,
                Resident {
                    meta,
                    index,
                    file,
                    lease: Weak::new(),
                },
            );
        }
        Ok(self.versions.get_mut(&id).unwrap())
    }
    fn version(&mut self, id: i64) -> Result<Version> {
        let value = self.load(id)?;
        let lease = value.lease.upgrade().unwrap_or_else(|| {
            Arc::new(Lease {
                valid: AtomicBool::new(true),
            })
        });
        value.lease = Arc::downgrade(&lease);
        Ok(Version {
            id,
            meta: value.meta.clone(),
            _lease: lease,
        })
    }
    fn lookup(&mut self, key: &str) -> Result<Option<Version>> {
        let mut row = self.db.statement(
            "SELECT id FROM versions WHERE resource_key=? AND obsolete=0 ORDER BY id DESC LIMIT 1",
            &[Value::Text(key)],
        )?;
        let id = if row.next()? { Some(row.int(0)) } else { None };
        drop(row);
        if let Some(id) = id {
            match self.version(id) {
                Ok(version) => {
                    self.db
                        .statement(
                            "UPDATE versions SET last_use=? WHERE id=?",
                            &[Value::Int(super::policy::now() as i64), Value::Int(id)],
                        )?
                        .next()?;
                    return Ok(Some(version));
                }
                Err(err) => {
                    log::warn!("discarding invalid cache version {id}: {err}");
                    self.delete(id)?;
                }
            }
        }
        Ok(None)
    }
    fn create(&mut self, meta: Metadata) -> Result<Version> {
        ensure!(valid_generation(&meta.generation), "invalid generation");
        self.reserve_index(meta.geometry.count() as usize * format::ENTRY_LEN)?;
        self.evict(HEADER_LEN)?;
        let path = self.path(&meta.generation);
        let mut file = OpenOptions::new()
            .create_new(true)
            .read(true)
            .write(true)
            .open(&path)?;
        file.write_all(&meta.geometry.header(&meta.generation)?)?;
        file.sync_all()?;
        sync_directory(&self.root)?;
        self.sizes.insert(
            meta.generation.clone(),
            (allocated(&file.metadata()?), meta.kind),
        );
        let index = Index::new(meta.geometry);
        let encoded = serde_json::to_string(&meta)?;
        self.db.exec("BEGIN IMMEDIATE")?;
        let result = (|| {
            self.db
                .statement(
                    "UPDATE versions SET obsolete=1 WHERE resource_key=?",
                    &[Value::Text(&meta.key)],
                )?
                .next()?;
            self.db.statement("INSERT INTO versions(resource_key,provider,kind,generation,meta,blocks,last_use) VALUES(?,?,?,?,?,?,?)", &[Value::Text(&meta.key),Value::Int(meta.provider),Value::Int(i64::from(meta.kind)),Value::Text(&meta.generation),Value::Text(&encoded),Value::Blob(index.bytes()),Value::Int(super::policy::now() as i64)])?.next()?;
            let id = self.db.insert_id();
            self.db.exec("COMMIT")?;
            Ok(id)
        })();
        let id = match result {
            Ok(id) => id,
            Err(err) => {
                let _ = self.db.exec("ROLLBACK");
                let _ = fs::remove_file(path);
                self.sizes.remove(&meta.generation);
                return Err(err);
            }
        };
        self.versions.insert(
            id,
            Resident {
                meta,
                index,
                file,
                lease: Weak::new(),
            },
        );
        self.version(id)
    }
    fn read(&mut self, id: i64, block: u64) -> Result<Option<Bytes>> {
        self.check_active(id)?;
        let value = self.load(id)?;
        let Some((slot, digest)) = value.index.get(block) else {
            return Ok(None);
        };
        let range = value.meta.geometry.range(block)?;
        let mut data = vec![0; (range.end - range.start) as usize];
        value
            .file
            .seek(SeekFrom::Start(value.meta.geometry.offset(slot)?))?;
        if value.file.read_exact(&mut data).is_err() || format::hash(&data)? != digest {
            value.index.remove(block);
            self.commit(id)?;
            return Ok(None);
        }
        Ok(Some(Bytes::from(data)))
    }
    fn write(&mut self, id: i64, blocks: &[(u64, Bytes)]) -> Result<()> {
        let result = self.write_data(id, blocks);
        if let Some(value) = self.versions.get(&id) {
            self.sizes.insert(
                value.meta.generation.clone(),
                (allocated(&value.file.metadata()?), value.meta.kind),
            );
        }
        result
    }
    fn write_data(&mut self, id: i64, blocks: &[(u64, Bytes)]) -> Result<()> {
        self.check_active(id)?;
        #[cfg(test)]
        if self.fault == Some(Fault::Write) {
            self.fault = None;
            anyhow::bail!("injected write failure");
        }
        let resident = self.load(id)?;
        let required = blocks
            .iter()
            .filter(|(i, _)| resident.index.get(*i).is_none())
            .count() as u64
            * resident.meta.geometry.block;
        self.evict(required)?;
        #[cfg(test)]
        let fail_sync = self.fault == Some(Fault::Sync);
        #[cfg(test)]
        let crash_sync = self.fault == Some(Fault::CrashSync);
        #[cfg(test)]
        if fail_sync {
            self.fault = None;
        }
        let value = self.load(id)?;
        let mut appended = Vec::new();
        for (block, data) in blocks {
            if value.index.get(*block).is_some() {
                continue;
            }
            let range = value.meta.geometry.range(*block)?;
            ensure!(
                data.len() as u64 == range.end - range.start,
                "incomplete block"
            );
            let size = value.file.metadata()?.len();
            let slots = (size - HEADER_LEN).div_ceil(value.meta.geometry.block);
            let slot = u32::try_from(slots)?;
            ensure!(slot != u32::MAX, "slot limit");
            let offset = value.meta.geometry.offset(slot)?;
            value.file.seek(SeekFrom::Start(offset))?;
            value.file.write_all(data)?;
            if (data.len() as u64) < value.meta.geometry.block {
                value.file.write_all(&vec![
                    0;
                    (value.meta.geometry.block - data.len() as u64)
                        as usize
                ])?;
            }
            appended.push((*block, slot, data));
        }
        #[cfg(test)]
        if fail_sync {
            anyhow::bail!("injected sync failure");
        }
        value.file.sync_data()?;
        #[cfg(test)]
        if crash_sync {
            std::process::exit(42);
        }
        for (block, slot, data) in appended {
            value.index.set(block, slot, data)?;
        }
        self.commit(id)
    }
    fn commit(&mut self, id: i64) -> Result<()> {
        let value = self
            .versions
            .get_mut(&id)
            .ok_or_else(|| anyhow::anyhow!("version not resident"))?;
        let ranges = value.index.dirty();
        if ranges.is_empty() {
            return Ok(());
        }
        self.db.exec("BEGIN IMMEDIATE")?;
        let result = self
            .db
            .patch(id, value.index.bytes(), &ranges)
            .and_then(|_| {
                #[cfg(test)]
                if self.fault == Some(Fault::CrashSql) {
                    std::process::exit(42);
                }
                #[cfg(test)]
                if self.fault == Some(Fault::Sql) {
                    self.fault = None;
                    anyhow::bail!("injected SQL failure");
                }
                self.db.exec("COMMIT")
            });
        if let Err(err) = result {
            let _ = self.db.exec("ROLLBACK");
            return Err(err);
        }
        value.index.committed();
        Ok(())
    }
    fn update(&mut self, id: i64, meta: Metadata) -> Result<()> {
        self.db
            .statement(
                "UPDATE versions SET meta=? WHERE id=?",
                &[Value::Text(&serde_json::to_string(&meta)?), Value::Int(id)],
            )?
            .next()?;
        self.load(id)?.meta = meta;
        Ok(())
    }
    fn invalidate(&mut self, provider: i64) -> Result<()> {
        for version in self
            .versions
            .values()
            .filter(|v| v.meta.provider == provider)
        {
            if let Some(lease) = version.lease.upgrade() {
                lease.valid.store(false, Ordering::Release);
            }
        }
        self.db
            .statement(
                "UPDATE versions SET obsolete=2 WHERE provider=?",
                &[Value::Int(provider)],
            )?
            .next()?;
        self.evict(0)
    }
    fn check_active(&self, id: i64) -> Result<()> {
        let mut row = self.db.statement(
            "SELECT obsolete FROM versions WHERE id=?",
            &[Value::Int(id)],
        )?;
        ensure!(row.next()? && row.int(0) != 2, "provider invalidated");
        Ok(())
    }
    fn reserve_index(&mut self, required: usize) -> Result<()> {
        const BUDGET: usize = 64 * 1024 * 1024;
        loop {
            let used: usize = self.versions.values().map(|v| v.index.bytes().len()).sum();
            if used + required <= BUDGET && self.versions.len() < MAX_RESIDENT {
                return Ok(());
            }
            let old = self
                .versions
                .iter()
                .find(|(_, v)| v.lease.upgrade().is_none())
                .map(|(id, _)| *id)
                .ok_or_else(|| anyhow::anyhow!("resource index budget reached"))?;
            self.commit(old)?;
            self.versions.remove(&old);
        }
    }
    fn delete(&mut self, id: i64) -> Result<()> {
        if self
            .versions
            .get(&id)
            .is_some_and(|v| v.lease.upgrade().is_some())
        {
            return Ok(());
        }
        let mut row = self.db.statement(
            "SELECT generation FROM versions WHERE id=?",
            &[Value::Int(id)],
        )?;
        let generation = if row.next()? {
            Some(row.text(0)?)
        } else {
            None
        };
        drop(row);
        self.db
            .statement("DELETE FROM versions WHERE id=?", &[Value::Int(id)])?
            .next()?;
        self.versions.remove(&id);
        if let Some(generation) = generation.filter(|g| valid_generation(g)) {
            match fs::remove_file(self.path(&generation)) {
                Ok(()) => (),
                Err(e) if e.kind() == std::io::ErrorKind::NotFound => (),
                Err(e) => return Err(e.into()),
            }
            self.sizes.remove(&generation);
        }
        Ok(())
    }
    fn usage(&mut self) -> Result<Usage> {
        let mut usage = Usage::default();
        for (size, kind) in self.sizes.values() {
            if *kind == 1 {
                usage.audio += size;
            } else {
                usage.image += size;
            }
        }
        for suffix in ["", "-wal", "-shm"] {
            match fs::metadata(self.root.join(format!("meta.sqlite{suffix}"))) {
                Ok(meta) => usage.metadata += allocated(&meta),
                Err(err) if err.kind() == std::io::ErrorKind::NotFound => (),
                Err(err) => return Err(err.into()),
            }
        }
        Ok(usage)
    }
    fn evict(&mut self, required: u64) -> Result<()> {
        let usage = self.usage()?;
        let over_capacity = usage.image + usage.audio + usage.metadata + required > self.capacity;
        let query = if over_capacity {
            "SELECT id, obsolete FROM versions ORDER BY obsolete DESC,last_use ASC"
        } else {
            "SELECT id, obsolete FROM versions WHERE obsolete<>0"
        };
        let mut rows = self.db.statement(query, &[])?;
        let mut ids = Vec::new();
        while rows.next()? {
            ids.push((rows.int(0), rows.int(1) != 0));
        }
        drop(rows);
        let usage = self.usage()?;
        let mut bytes = usage.image + usage.audio + usage.metadata;
        for (id, obsolete) in ids {
            if !obsolete && bytes.saturating_add(required) <= self.capacity {
                continue;
            }
            if self
                .versions
                .get(&id)
                .is_some_and(|v| v.lease.upgrade().is_some())
            {
                continue;
            }
            self.delete(id)?;
            let usage = self.usage()?;
            bytes = usage.image + usage.audio + usage.metadata;
        }
        ensure!(
            bytes.saturating_add(required) <= self.capacity,
            "cache capacity reached"
        );
        Ok(())
    }
}
fn valid_generation(value: &str) -> bool {
    value.len() == 32 && value.bytes().all(|b| b.is_ascii_hexdigit())
}
fn allocated(metadata: &fs::Metadata) -> u64 {
    #[cfg(unix)]
    {
        use std::os::unix::fs::MetadataExt;
        metadata.blocks().saturating_mul(512)
    }
    #[cfg(not(unix))]
    {
        metadata.len()
    }
}
fn sync_directory(path: &Path) -> Result<()> {
    #[cfg(unix)]
    File::open(path)?.sync_all()?;
    Ok(())
}
