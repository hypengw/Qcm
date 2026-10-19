use anyhow::{ensure, Result};
use libsqlite3_sys::*;
use std::{
    ffi::{CStr, CString},
    path::Path,
    ptr,
};

pub struct Database {
    handle: *mut sqlite3,
}
pub enum Value<'a> {
    Int(i64),
    Text(&'a str),
    Blob(&'a [u8]),
}
pub struct Statement<'a> {
    handle: *mut sqlite3_stmt,
    db: &'a Database,
}

impl Database {
    pub fn open(path: &Path) -> Result<Self> {
        let path = CString::new(
            path.to_str()
                .ok_or_else(|| anyhow::anyhow!("invalid cache path"))?,
        )?;
        let mut handle = ptr::null_mut();
        // This connection belongs exclusively to the storage thread.
        let code = unsafe {
            sqlite3_open_v2(
                path.as_ptr(),
                &mut handle,
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                ptr::null(),
            )
        };
        let db = Self { handle };
        db.check(code)?;
        db.exec("PRAGMA journal_mode=WAL; PRAGMA synchronous=FULL; PRAGMA busy_timeout=5000;")?;
        Ok(db)
    }
    fn check(&self, code: i32) -> Result<()> {
        ensure!(
            matches!(code, SQLITE_OK | SQLITE_ROW | SQLITE_DONE),
            "sqlite {code}: {}",
            if self.handle.is_null() {
                "open failed".into()
            } else {
                unsafe { CStr::from_ptr(sqlite3_errmsg(self.handle)) }.to_string_lossy()
            }
        );
        Ok(())
    }
    pub fn exec(&self, sql: &str) -> Result<()> {
        let sql = CString::new(sql)?;
        self.check(unsafe {
            sqlite3_exec(
                self.handle,
                sql.as_ptr(),
                None,
                ptr::null_mut(),
                ptr::null_mut(),
            )
        })
    }
    pub fn statement(&self, sql: &str, values: &[Value<'_>]) -> Result<Statement<'_>> {
        let sql = CString::new(sql)?;
        let mut handle = ptr::null_mut();
        self.check(unsafe {
            sqlite3_prepare_v2(self.handle, sql.as_ptr(), -1, &mut handle, ptr::null_mut())
        })?;
        let statement = Statement { handle, db: self };
        for (index, value) in values.iter().enumerate() {
            let i = (index + 1) as i32;
            let code = unsafe {
                match value {
                    Value::Int(v) => sqlite3_bind_int64(handle, i, *v),
                    Value::Text(v) => sqlite3_bind_text(
                        handle,
                        i,
                        v.as_ptr().cast(),
                        i32::try_from(v.len())?,
                        SQLITE_TRANSIENT(),
                    ),
                    Value::Blob(v) => sqlite3_bind_blob(
                        handle,
                        i,
                        v.as_ptr().cast(),
                        i32::try_from(v.len())?,
                        SQLITE_TRANSIENT(),
                    ),
                }
            };
            self.check(code)?;
        }
        Ok(statement)
    }
    pub fn insert_id(&self) -> i64 {
        unsafe { sqlite3_last_insert_rowid(self.handle) }
    }
    pub fn patch(&self, row: i64, bytes: &[u8], ranges: &[std::ops::Range<usize>]) -> Result<()> {
        let mut handle = ptr::null_mut();
        self.check(unsafe {
            sqlite3_blob_open(
                self.handle,
                c"main".as_ptr(),
                c"versions".as_ptr(),
                c"blocks".as_ptr(),
                row,
                1,
                &mut handle,
            )
        })?;
        let result = (|| {
            ensure!(
                unsafe { sqlite3_blob_bytes(handle) } as usize == bytes.len(),
                "BLOB length changed"
            );
            for range in ranges {
                self.check(unsafe {
                    sqlite3_blob_write(
                        handle,
                        bytes[range.clone()].as_ptr().cast(),
                        i32::try_from(range.len())?,
                        i32::try_from(range.start)?,
                    )
                })?;
            }
            Ok(())
        })();
        let closed = self.check(unsafe { sqlite3_blob_close(handle) });
        result.and(closed)
    }
}
impl Drop for Database {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                sqlite3_close(self.handle);
            }
        }
    }
}
impl Statement<'_> {
    pub fn next(&mut self) -> Result<bool> {
        let code = unsafe { sqlite3_step(self.handle) };
        self.db.check(code)?;
        Ok(code == SQLITE_ROW)
    }
    pub fn int(&self, column: i32) -> i64 {
        unsafe { sqlite3_column_int64(self.handle, column) }
    }
    pub fn blob(&self, column: i32) -> Vec<u8> {
        let length = unsafe { sqlite3_column_bytes(self.handle, column) } as usize;
        if length == 0 {
            return Vec::new();
        }
        unsafe {
            std::slice::from_raw_parts(
                sqlite3_column_blob(self.handle, column).cast::<u8>(),
                length,
            )
        }
        .to_vec()
    }
    pub fn text(&self, column: i32) -> Result<String> {
        Ok(String::from_utf8(self.blob(column))?)
    }
}
impl Drop for Statement<'_> {
    fn drop(&mut self) {
        unsafe {
            sqlite3_finalize(self.handle);
        }
    }
}

pub fn migrate(db: &Database) -> Result<()> {
    let mut version = db.statement("PRAGMA user_version", &[])?;
    version.next()?;
    ensure!(version.int(0) <= 4, "unsupported resource cache schema");
    drop(version);
    db.exec("BEGIN IMMEDIATE")?;
    let result = (|| {
        db.exec("CREATE TABLE IF NOT EXISTS versions (id INTEGER PRIMARY KEY, resource_key TEXT NOT NULL, provider INTEGER NOT NULL, kind INTEGER NOT NULL, generation TEXT NOT NULL UNIQUE, meta TEXT NOT NULL, blocks BLOB NOT NULL, last_use INTEGER NOT NULL, obsolete INTEGER NOT NULL DEFAULT 0); CREATE INDEX IF NOT EXISTS resource_lookup ON versions(resource_key, id);")?;
        db.exec("CREATE TABLE IF NOT EXISTS complete_files(version_id INTEGER PRIMARY KEY, ready INTEGER NOT NULL DEFAULT 0, length INTEGER NOT NULL DEFAULT 0, digest BLOB, retry_at INTEGER NOT NULL DEFAULT 0, format INTEGER NOT NULL DEFAULT 1); CREATE TABLE IF NOT EXISTS file_leases(token TEXT PRIMARY KEY, version_id INTEGER NOT NULL, client TEXT NOT NULL, expires INTEGER NOT NULL); CREATE INDEX IF NOT EXISTS lease_versions ON file_leases(version_id); CREATE TABLE IF NOT EXISTS file_clock(id INTEGER PRIMARY KEY CHECK(id=1), time INTEGER NOT NULL);")?;
        let mut columns = db.statement("PRAGMA table_info(complete_files)", &[])?;
        let mut has_format = false;
        while columns.next()? {
            has_format |= columns.text(1)? == "format";
        }
        drop(columns);
        if !has_format {
            db.exec("ALTER TABLE complete_files ADD COLUMN format INTEGER NOT NULL DEFAULT 1")?;
        }
        db.exec("CREATE TABLE IF NOT EXISTS segments(version_id INTEGER NOT NULL, number INTEGER NOT NULL, token TEXT NOT NULL, length INTEGER NOT NULL, last_use INTEGER NOT NULL, deleting INTEGER NOT NULL DEFAULT 0, PRIMARY KEY(version_id,number)); CREATE INDEX IF NOT EXISTS segment_lru ON segments(deleting,last_use); CREATE TABLE IF NOT EXISTS packs(token TEXT PRIMARY KEY,group_key TEXT NOT NULL,provider INTEGER NOT NULL,length INTEGER NOT NULL,last_use INTEGER NOT NULL,deleting INTEGER NOT NULL DEFAULT 0); CREATE INDEX IF NOT EXISTS pack_group ON packs(group_key,deleting,last_use); CREATE TABLE IF NOT EXISTS objects(resource_key TEXT PRIMARY KEY,pack TEXT NOT NULL,offset INTEGER NOT NULL,length INTEGER NOT NULL,digest BLOB NOT NULL,meta TEXT NOT NULL); CREATE INDEX IF NOT EXISTS object_pack ON objects(pack); PRAGMA user_version=4; COMMIT;")
    })();
    if result.is_err() {
        let _ = db.exec("ROLLBACK");
    }
    result
}
