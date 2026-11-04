use sea_orm::{ConnectionTrait, DatabaseConnection, DbBackend, QueryResult, Statement};
use serde::Serialize;

pub const MAX_STATE_BYTES: usize = 1024 * 1024;

#[derive(Clone, Debug, PartialEq, Eq, Serialize)]
pub struct InstallationState {
    pub revision: u64,
    pub data: Option<String>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize)]
pub struct StateUpdate {
    pub applied: bool,
    pub state: InstallationState,
}

fn decode(row: QueryResult) -> crate::Result<InstallationState> {
    let revision: i64 = row.try_get("", "revision")?;
    if revision <= 0 {
        return Err(crate::anyhow!("invalid plugin state revision"));
    }
    let data: String = row.try_get("", "data")?;
    if data.len() > MAX_STATE_BYTES {
        return Err(crate::anyhow!("plugin state exceeds size limit"));
    }
    Ok(InstallationState {
        revision: revision as u64,
        data: Some(data),
    })
}

pub(super) async fn load(
    db: &DatabaseConnection,
    namespace: &str,
) -> crate::Result<InstallationState> {
    match db
        .query_one(Statement::from_sql_and_values(
            DbBackend::Sqlite,
            "SELECT revision, data FROM plugin_installation_state WHERE namespace = ?",
            [namespace.into()],
        ))
        .await?
    {
        Some(row) => decode(row),
        None => Ok(InstallationState {
            revision: 0,
            data: None,
        }),
    }
}

pub(super) async fn compare_exchange(
    db: &DatabaseConnection,
    namespace: &str,
    revision: u64,
    data: String,
) -> crate::Result<StateUpdate> {
    if data.len() > MAX_STATE_BYTES {
        return Err(crate::anyhow!("plugin state exceeds size limit"));
    }
    let revision = i64::try_from(revision)
        .ok()
        .filter(|r| *r < i64::MAX)
        .ok_or_else(|| crate::anyhow!("plugin state revision is exhausted"))?;
    let statement = if revision == 0 {
        Statement::from_sql_and_values(
            DbBackend::Sqlite,
            "INSERT INTO plugin_installation_state(namespace, revision, data) VALUES (?, 1, ?) \
             ON CONFLICT(namespace) DO NOTHING RETURNING revision, data",
            [namespace.into(), data.into()],
        )
    } else {
        Statement::from_sql_and_values(
            DbBackend::Sqlite,
            "UPDATE plugin_installation_state SET revision = revision + 1, data = ? \
             WHERE namespace = ? AND revision = ? RETURNING revision, data",
            [data.into(), namespace.into(), revision.into()],
        )
    };
    match db.query_one(statement).await? {
        Some(row) => Ok(StateUpdate {
            applied: true,
            state: decode(row)?,
        }),
        None => Ok(StateUpdate {
            applied: false,
            state: load(db, namespace).await?,
        }),
    }
}
