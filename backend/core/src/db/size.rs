use sea_orm::{ConnectionTrait, DatabaseBackend, DatabaseConnection, DbErr, Statement, TryGetable};
use std::fs;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone)]
pub struct DbFileSize {
    pub name: String,
    pub file: Option<PathBuf>,
    pub logical_bytes: u64,
    pub disk_bytes: u64,
}

#[derive(Debug, Clone)]
pub struct DbSizes {
    pub per_db: Vec<DbFileSize>,
    pub total_logical_bytes: u64,
    pub total_disk_bytes: u64,
}

pub async fn sqlite_sizes(db: &DatabaseConnection) -> Result<DbSizes, DbErr> {
    let dbs = database_list(db).await?;

    let mut per_db = Vec::with_capacity(dbs.len());
    let mut total_logical = 0u64;
    let mut total_disk = 0u64;

    for (name, file) in dbs {
        let page_count: i64 = pragma_i64(db, &format!("PRAGMA {name}.page_count"))
            .await?
            .unwrap_or(0);
        let page_size: i64 = pragma_i64(db, &format!("PRAGMA {name}.page_size"))
            .await?
            .unwrap_or(0);

        let logical = if page_count > 0 && page_size > 0 {
            (page_count as u64).saturating_mul(page_size as u64)
        } else {
            0
        };

        let disk = if let Some(ref p) = file {
            size_if_exists(p)
                .saturating_add(size_if_exists(&with_suffix(p, "-wal")))
                .saturating_add(size_if_exists(&with_suffix(p, "-shm")))
                .saturating_add(size_if_exists(&with_suffix(p, "-journal")))
        } else {
            0
        };

        total_logical = total_logical.saturating_add(logical);
        total_disk = total_disk.saturating_add(disk);

        per_db.push(DbFileSize {
            name,
            file,
            logical_bytes: logical,
            disk_bytes: disk,
        });
    }

    Ok(DbSizes {
        per_db,
        total_logical_bytes: total_logical,
        total_disk_bytes: total_disk,
    })
}

async fn database_list(db: &DatabaseConnection) -> Result<Vec<(String, Option<PathBuf>)>, DbErr> {
    // PRAGMA database_list 返回列：seq | name | file
    let stmt = Statement::from_string(DatabaseBackend::Sqlite, "PRAGMA database_list".to_owned());
    let rows = db.query_all(stmt).await?;

    let mut out = Vec::with_capacity(rows.len());
    for row in rows {
        let name: String = row.try_get("", "name").unwrap_or_default();
        let file: Option<String> = row.try_get("", "file").ok();
        let file = file.and_then(|s| {
            if s.is_empty() {
                None
            } else {
                Some(PathBuf::from(s))
            }
        });
        out.push((name, file));
    }
    Ok(out)
}

async fn pragma_i64(db: &DatabaseConnection, sql: &str) -> Result<Option<i64>, DbErr> {
    let stmt = Statement::from_string(DatabaseBackend::Sqlite, sql.to_owned());
    let row_opt = db.query_one(stmt).await?;
    if let Some(row) = row_opt {
        // 列名就是 pragma 名本身或匿名列，这里尝试几个常见键
        for key in [
            "page_count",
            "page_size",
            "cache_size",
            "wal_autocheckpoint",
            "freelist_count",
            "",
        ] {
            if let Ok(v) = row.try_get::<i64>("", key) {
                return Ok(Some(v));
            }
        }
        // 兜底遍历所有列（极少需要）
        for idx in 0.. {
            if let Ok(v) = row.try_get_by_index::<i64>(idx) {
                return Ok(Some(v));
            } else {
                break;
            }
        }
        Ok(None)
    } else {
        Ok(None)
    }
}

fn with_suffix(p: &Path, suffix: &str) -> PathBuf {
    let s = p.to_string_lossy();
    PathBuf::from(format!("{s}{suffix}"))
}

fn size_if_exists(p: &Path) -> u64 {
    fs::metadata(p).map(|m| m.len()).unwrap_or(0)
}
