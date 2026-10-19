use super::super::{
    format,
    policy::{Headers, Policy},
    sqlite::Value,
};
use super::{allocated, materialize, sync_directory, Storage, Store};
use anyhow::{ensure, Result};
use bytes::Bytes;
use serde::{Deserialize, Serialize};
use std::{
    collections::HashMap,
    fs::{self, OpenOptions},
    io::{Read, Seek, SeekFrom, Write},
};

pub const PACK_BYTES: u64 = 16 * 1024 * 1024;
const HEADER: u64 = 64;
pub const MAX_OBJECT: u64 = PACK_BYTES - HEADER;

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ObjectMetadata {
    pub key: String,
    pub group: String,
    pub provider: i64,
    pub length: u64,
    pub headers: Headers,
    pub policy: Policy,
    pub target: String,
    pub effective_url: String,
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    #[ignore = "subprocess crash helper"]
    fn pack_crash_worker() {
        let Some(root) = std::env::var_os("QCM_PACK_CRASH_ROOT") else {
            return;
        };
        let mut store = Storage::open(root.into()).unwrap();
        let point = std::env::var("QCM_PACK_CRASH_POINT").unwrap();
        store.fault = Some(match point.as_str() {
            "sync" => super::super::Fault::CrashSync,
            "sql" => super::super::Fault::CrashSql,
            "committed" => super::super::Fault::CrashSegmentCommit,
            "deleting" => super::super::Fault::CrashSegmentDelete,
            "unlinked" => super::super::Fault::CrashSegmentUnlink,
            _ => panic!("unknown crash point"),
        });
        if matches!(point.as_str(), "deleting" | "unlinked") {
            store.capacity = 0;
            let _ = store.evict(0);
        } else {
            let _ = store.put_object(meta("new", 3), b"new");
        }
        panic!("crash point did not exit");
    }
    #[test]
    fn packed_objects_crash_recovery_respects_commits_and_deletion() {
        for point in ["sync", "sql", "committed", "deleting", "unlinked"] {
            let directory = Directory::new();
            let mut store = Storage::open(directory.0.clone()).unwrap();
            store.put_object(meta("old", 3), b"old").unwrap();
            drop(store);
            let status = std::process::Command::new(std::env::current_exe().unwrap())
                .args([
                    "--exact",
                    "resource_cache::store::objects::tests::pack_crash_worker",
                    "--ignored",
                ])
                .env("QCM_PACK_CRASH_ROOT", &directory.0)
                .env("QCM_PACK_CRASH_POINT", point)
                .status()
                .unwrap();
            assert_eq!(status.code(), Some(42));
            let mut store = Storage::open(directory.0.clone()).unwrap();
            let deleting = matches!(point, "deleting" | "unlinked");
            assert_eq!(store.object("old").unwrap().is_some(), !deleting);
            assert_eq!(store.object("new").unwrap().is_some(), point == "committed");
            if deleting {
                assert_eq!(store.usage().unwrap().audio, 0);
            }
        }
    }
    struct Directory(std::path::PathBuf);
    impl Directory {
        fn new() -> Self {
            Self(std::env::temp_dir().join(format!("qcm-packs-{}", uuid::Uuid::new_v4())))
        }
    }
    impl Drop for Directory {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.0);
        }
    }
    fn meta(key: &str, length: u64) -> ObjectMetadata {
        ObjectMetadata {
            key: key.into(),
            group: "one-output".into(),
            provider: 1,
            length,
            headers: vec![],
            policy: Policy::from_response(&Default::default(), 0, 0),
            target: "origin".into(),
            effective_url: "http://example.test/media".into(),
        }
    }
    #[test]
    fn packed_objects_share_files_recover_and_isolate_corruption() {
        let dir = Directory::new();
        let mut store = Storage::open(dir.0.clone()).unwrap();
        store.put_object(meta("a", 10), b"0123456789").unwrap();
        store.put_object(meta("b", 11), b"abcdefghijk").unwrap();
        assert_eq!(
            store.object("a").unwrap().unwrap().bytes.as_ref(),
            b"0123456789"
        );
        let packs: Vec<_> = fs::read_dir(&dir.0)
            .unwrap()
            .map(|e| e.unwrap().path())
            .filter(|p| p.extension().is_some_and(|x| x == "pack"))
            .collect();
        assert_eq!(packs.len(), 1);
        let mut file = OpenOptions::new().write(true).open(&packs[0]).unwrap();
        file.seek(SeekFrom::End(0)).unwrap();
        file.write_all(b"uncommitted tail").unwrap();
        drop(file);
        drop(store);
        let mut store = Storage::open(dir.0.clone()).unwrap();
        assert_eq!(fs::metadata(&packs[0]).unwrap().len(), HEADER + 21);
        let mut file = OpenOptions::new().write(true).open(&packs[0]).unwrap();
        file.seek(SeekFrom::Start(HEADER)).unwrap();
        file.write_all(b"!").unwrap();
        drop(file);
        assert!(store.object("a").unwrap().is_none());
        assert_eq!(
            store.object("b").unwrap().unwrap().bytes.as_ref(),
            b"abcdefghijk"
        );
        drop(store);
        fs::remove_file(&packs[0]).unwrap();
        let mut store = Storage::open(dir.0.clone()).unwrap();
        assert!(store.object("b").unwrap().is_none());
    }
    #[test]
    fn packed_objects_publish_only_after_durable_commit() {
        let dir = Directory::new();
        let mut store = Storage::open(dir.0.clone()).unwrap();
        store.put_object(meta("old", 3), b"old").unwrap();
        for fault in [
            super::super::Fault::Write,
            super::super::Fault::Sync,
            super::super::Fault::Sql,
        ] {
            store.fault = Some(fault);
            assert!(store.put_object(meta("new", 3), b"new").is_err());
            assert!(store.object("new").unwrap().is_none());
            assert_eq!(store.object("old").unwrap().unwrap().bytes.as_ref(), b"old");
        }
        drop(store);
        let mut store = Storage::open(dir.0.clone()).unwrap();
        assert!(store.object("old").unwrap().is_some());
        assert!(store.object("new").unwrap().is_none());
    }
    #[test]
    fn packed_objects_roll_over_evict_and_retain_deletion_accounting() {
        let dir = Directory::new();
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let bytes = vec![7; 9 * 1024 * 1024];
        store
            .put_object(meta("one", bytes.len() as u64), &bytes)
            .unwrap();
        store
            .put_object(meta("two", bytes.len() as u64), &bytes)
            .unwrap();
        let packs: Vec<_> = fs::read_dir(&dir.0)
            .unwrap()
            .map(|e| e.unwrap().path())
            .filter(|p| p.extension().is_some_and(|x| x == "pack"))
            .collect();
        assert_eq!(packs.len(), 2);
        assert!(packs
            .iter()
            .all(|p| fs::metadata(p).unwrap().len() <= PACK_BYTES));
        assert!(store.object("one").unwrap().is_some());
        let usage = store.usage().unwrap();
        store.capacity = usage.audio + usage.metadata - bytes.len() as u64 + 65536;
        store.evict(0).unwrap();
        assert!(store.object("one").unwrap().is_some());
        assert!(store.object("two").unwrap().is_none());
        store.fault = Some(super::super::Fault::SegmentDelete);
        assert!(store.invalidate_packs(1).is_ok());
        assert!(store.object("one").unwrap().is_none());
        assert!(store.usage().unwrap().audio > 0);
        store.capacity = u64::MAX;
        store.evict(0).unwrap();
        assert_eq!(store.usage().unwrap().audio, 0);
    }
}
pub struct Object {
    pub meta: ObjectMetadata,
    pub bytes: Bytes,
}

fn name(token: &str) -> String {
    format!("{token}.pack")
}
pub(super) fn managed_name(value: &str) -> bool {
    value
        .strip_suffix(".pack")
        .is_some_and(|s| s.len() == 32 && s.bytes().all(|c| c.is_ascii_hexdigit()))
}
fn header(token: &str) -> [u8; 64] {
    let mut bytes = [0; 64];
    bytes[..8].copy_from_slice(b"QCMPACK1");
    bytes[8..40].copy_from_slice(token.as_bytes());
    bytes
}

impl Store {
    pub async fn object(&self, key: String) -> Result<Option<Object>> {
        self.call(move |s| s.object(&key)).await
    }
    pub async fn put_object(
        &self,
        meta: ObjectMetadata,
        bytes: Bytes,
        valid: impl Fn() -> bool + Send + 'static,
    ) -> Result<()> {
        self.call(move |s| {
            ensure!(valid(), "object commit invalidated");
            let key = meta.key.clone();
            let result = s.put_object(meta, &bytes);
            if !valid() {
                s.db.statement(
                    "DELETE FROM objects WHERE resource_key=?",
                    &[Value::Text(&key)],
                )?
                .next()?;
                anyhow::bail!("object commit invalidated");
            }
            result
        })
        .await
    }
    pub async fn remove_object(&self, key: String) -> Result<()> {
        self.call(move |s| {
            s.db.statement(
                "DELETE FROM objects WHERE resource_key=?",
                &[Value::Text(&key)],
            )?
            .next()?;
            Ok(())
        })
        .await
    }
    pub async fn update_object(&self, meta: ObjectMetadata) -> Result<()> {
        self.call(move |s| {
            s.db.statement(
                "UPDATE objects SET meta=? WHERE resource_key=?",
                &[
                    Value::Text(&serde_json::to_string(&meta)?),
                    Value::Text(&meta.key),
                ],
            )?
            .next()?;
            Ok(())
        })
        .await
    }
}

impl Storage {
    fn object(&mut self, key: &str) -> Result<Option<Object>> {
        let mut row = self.db.statement("SELECT o.meta,o.offset,o.length,o.digest,p.token,p.length FROM objects o JOIN packs p ON o.pack=p.token WHERE o.resource_key=? AND p.deleting=0", &[Value::Text(key)])?;
        if !row.next()? {
            return Ok(None);
        }
        let meta: ObjectMetadata = serde_json::from_str(&row.text(0)?)?;
        let offset = u64::try_from(row.int(1))?;
        let length = u64::try_from(row.int(2))?;
        let digest = row.blob(3);
        let token = row.text(4)?;
        let committed = u64::try_from(row.int(5))?;
        drop(row);
        let result = (|| {
            ensure!(
                managed_name(&name(&token))
                    && offset >= HEADER
                    && length <= MAX_OBJECT
                    && meta.length == length
                    && offset.checked_add(length).is_some_and(|n| n <= committed),
                "invalid packed object"
            );
            let path = self.root.join(name(&token));
            materialize::regular_file(&path)?;
            let mut file = fs::File::open(path)?;
            let mut actual = [0; 64];
            file.read_exact(&mut actual)?;
            ensure!(actual == header(&token), "pack identity mismatch");
            file.seek(SeekFrom::Start(offset))?;
            let mut bytes = vec![0; length as usize];
            file.read_exact(&mut bytes)?;
            ensure!(
                format::hash(&bytes)?.as_slice() == digest,
                "object checksum mismatch"
            );
            Ok(Object {
                meta,
                bytes: bytes.into(),
            })
        })();
        match result {
            Ok(object) => {
                let clock = self.segment_tick();
                self.db
                    .statement(
                        "UPDATE packs SET last_use=? WHERE token=?",
                        &[Value::Int(clock as i64), Value::Text(&token)],
                    )?
                    .next()?;
                Ok(Some(object))
            }
            Err(err) => {
                log::warn!("packed object discarded: {err}");
                self.db
                    .statement(
                        "DELETE FROM objects WHERE resource_key=?",
                        &[Value::Text(key)],
                    )?
                    .next()?;
                Ok(None)
            }
        }
    }
    fn put_object(&mut self, meta: ObjectMetadata, bytes: &[u8]) -> Result<()> {
        ensure!(
            meta.length == bytes.len() as u64 && meta.length <= MAX_OBJECT,
            "object admission limit"
        );
        // Readers own complete verified bytes; no pack file remains pinned after a read.
        self.evict((meta.length + HEADER).div_ceil(4096) * 4096)?;
        let mut row = self.db.statement("SELECT token,length FROM packs WHERE group_key=? AND deleting=0 AND length+?<=? ORDER BY last_use DESC LIMIT 1", &[Value::Text(&meta.group),Value::Int(meta.length as i64),Value::Int(PACK_BYTES as i64)])?;
        let existing = if row.next()? {
            Some((row.text(0)?, u64::try_from(row.int(1))?))
        } else {
            None
        };
        drop(row);
        let (token, offset) = if let Some(value) = existing {
            value
        } else {
            let token = uuid::Uuid::new_v4().simple().to_string();
            let mut file = OpenOptions::new()
                .create_new(true)
                .read(true)
                .write(true)
                .open(self.root.join(name(&token)))?;
            let created = (|| -> Result<()> {
                materialize::private_file(&file)?;
                file.write_all(&header(&token))?;
                file.sync_all()?;
                sync_directory(&self.root)?;
                self.db.statement("INSERT INTO packs(token,group_key,provider,length,last_use) VALUES(?,?,?,?,?)",&[Value::Text(&token),Value::Text(&meta.group),Value::Int(meta.provider),Value::Int(HEADER as i64),Value::Int(0)])?.next()?;
                Ok(())
            })();
            self.sizes.insert(
                name(&token),
                (file.metadata().map(|m| allocated(&m)).unwrap_or(4096), 1),
            );
            drop(file);
            if let Err(error) = created {
                if let Err(cleanup) = self.remove_managed(&name(&token)) {
                    log::warn!("pack creation cleanup deferred: {cleanup}");
                }
                return Err(error);
            }
            (token, HEADER)
        };
        ensure!(
            managed_name(&name(&token)) && offset >= HEADER && offset + meta.length <= PACK_BYTES,
            "invalid pack append"
        );
        let path = self.root.join(name(&token));
        materialize::regular_file(&path)?;
        let mut file = OpenOptions::new().read(true).write(true).open(&path)?;
        let result = (|| {
            let mut actual = [0; 64];
            file.read_exact(&mut actual)?;
            ensure!(
                actual == header(&token) && file.metadata()?.len() >= offset,
                "invalid pack identity"
            );
            file.set_len(offset)?;
            file.seek(SeekFrom::Start(offset))?;
            file.write_all(bytes)?;
            #[cfg(test)]
            self.pack_fault(super::Fault::Write)?;
            file.sync_data()?;
            #[cfg(test)]
            self.pack_fault(super::Fault::Sync)?;
            #[cfg(test)]
            self.pack_fault(super::Fault::CrashSync)?;
            let digest = format::hash(bytes)?;
            let clock = self.segment_tick();
            self.db.exec("BEGIN IMMEDIATE")?;
            let committed = (|| {
                self.db.statement("INSERT OR REPLACE INTO objects(resource_key,pack,offset,length,digest,meta) VALUES(?,?,?,?,?,?)", &[Value::Text(&meta.key),Value::Text(&token),Value::Int(offset as i64),Value::Int(meta.length as i64),Value::Blob(&digest),Value::Text(&serde_json::to_string(&meta)?)])?.next()?;
                self.db
                    .statement(
                        "UPDATE packs SET length=?,last_use=? WHERE token=? AND deleting=0",
                        &[
                            Value::Int((offset + meta.length) as i64),
                            Value::Int(clock as i64),
                            Value::Text(&token),
                        ],
                    )?
                    .next()?;
                #[cfg(test)]
                self.pack_fault(super::Fault::Sql)?;
                #[cfg(test)]
                self.pack_fault(super::Fault::CrashSql)?;
                self.db.exec("COMMIT")
            })();
            if committed.is_err() {
                let _ = self.db.exec("ROLLBACK");
            }
            committed?;
            #[cfg(test)]
            self.pack_fault(super::Fault::CrashSegmentCommit)?;
            Ok(())
        })();
        if result.is_err() {
            let _ = file.set_len(offset);
            let _ = file.sync_data();
        }
        self.sizes
            .insert(name(&token), (allocated(&file.metadata()?), 1));
        result
    }
    #[cfg(test)]
    fn pack_fault(&mut self, fault: super::Fault) -> Result<()> {
        if self.fault == Some(fault) {
            if matches!(
                fault,
                super::Fault::CrashSync
                    | super::Fault::CrashSql
                    | super::Fault::CrashSegmentCommit
                    | super::Fault::CrashSegmentDelete
                    | super::Fault::CrashSegmentUnlink
            ) {
                std::process::exit(42);
            }
            self.fault = None;
            anyhow::bail!("injected pack failure");
        }
        Ok(())
    }
    fn delete_pack(&mut self, token: &str) -> Result<()> {
        ensure!(managed_name(&name(token)), "invalid pack token");
        self.db.exec("BEGIN IMMEDIATE")?;
        let result = (|| {
            self.db
                .statement("DELETE FROM objects WHERE pack=?", &[Value::Text(token)])?
                .next()?;
            self.db
                .statement(
                    "UPDATE packs SET deleting=1 WHERE token=?",
                    &[Value::Text(token)],
                )?
                .next()?;
            self.db.exec("COMMIT")
        })();
        if result.is_err() {
            let _ = self.db.exec("ROLLBACK");
        }
        result?;
        #[cfg(test)]
        self.pack_fault(super::Fault::SegmentDelete)?;
        #[cfg(test)]
        self.pack_fault(super::Fault::CrashSegmentDelete)?;
        self.remove_managed(&name(token))?;
        #[cfg(test)]
        self.pack_fault(super::Fault::CrashSegmentUnlink)?;
        self.db
            .statement("DELETE FROM packs WHERE token=?", &[Value::Text(token)])?
            .next()?;
        Ok(())
    }
    pub(super) fn retain_packs(&mut self, retained: &mut HashMap<String, i32>) -> Result<()> {
        let mut rows = self
            .db
            .statement("SELECT token,length,deleting FROM packs", &[])?;
        let mut packs = Vec::new();
        while rows.next()? {
            packs.push((rows.text(0)?, rows.int(1), rows.int(2)));
        }
        drop(rows);
        for (token, length, deleting) in packs {
            let valid = (|| -> Result<()> {
                ensure!(
                    managed_name(&name(&token))
                        && (HEADER..=PACK_BYTES).contains(&(length as u64))
                        && deleting == 0,
                    "invalid pack descriptor"
                );
                let path = self.root.join(name(&token));
                materialize::regular_file(&path)?;
                let mut file = OpenOptions::new().read(true).write(true).open(path)?;
                let mut actual = [0; 64];
                file.read_exact(&mut actual)?;
                ensure!(
                    actual == header(&token) && file.metadata()?.len() >= length as u64,
                    "invalid pack data"
                );
                file.set_len(length as u64)?;
                Ok(())
            })();
            if valid.is_ok() {
                retained.insert(name(&token), 1);
            } else if managed_name(&name(&token)) {
                let _ = self.delete_pack(&token);
            } else {
                self.db
                    .statement("DELETE FROM objects WHERE pack=?", &[Value::Text(&token)])?
                    .next()?;
                self.db
                    .statement("DELETE FROM packs WHERE token=?", &[Value::Text(&token)])?
                    .next()?;
            }
        }
        Ok(())
    }
    pub(super) fn evict_packs(&mut self, required: u64) -> Result<()> {
        let mut rows = self.db.statement(
            "SELECT token,deleting FROM packs ORDER BY deleting DESC,last_use ASC",
            &[],
        )?;
        let mut packs = Vec::new();
        while rows.next()? {
            packs.push((rows.text(0)?, rows.int(1)));
        }
        drop(rows);
        for (token, deleting) in packs {
            let usage = self.usage()?;
            if deleting == 0
                && usage
                    .audio
                    .saturating_add(usage.image)
                    .saturating_add(usage.metadata)
                    .saturating_add(required)
                    <= self.capacity
            {
                break;
            }
            if let Err(err) = self.delete_pack(&token) {
                log::warn!("pack deletion deferred: {err}");
            }
        }
        Ok(())
    }
    pub(super) fn invalidate_packs(&mut self, provider: i64) -> Result<()> {
        let mut rows = self.db.statement(
            "SELECT token FROM packs WHERE provider=?",
            &[Value::Int(provider)],
        )?;
        let mut tokens = Vec::new();
        while rows.next()? {
            tokens.push(rows.text(0)?);
        }
        drop(rows);
        for token in tokens {
            if let Err(err) = self.delete_pack(&token) {
                log::warn!("pack invalidation deferred: {err}");
            }
        }
        Ok(())
    }
}
