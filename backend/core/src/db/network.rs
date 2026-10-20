use crate::network::{NetworkSettings, ProxyEntry};
use sea_orm::{ConnectionTrait, DbErr, Statement};

fn stmt(sql: &str, values: Vec<sea_orm::Value>) -> Statement {
    Statement::from_sql_and_values(sea_orm::DbBackend::Sqlite, sql, values)
}
pub async fn load(db: &impl ConnectionTrait) -> Result<NetworkSettings, DbErr> {
    let row = db
        .query_one(stmt(
            "SELECT revision FROM network_settings WHERE id=1",
            vec![],
        ))
        .await?
        .ok_or_else(|| DbErr::Custom("Network settings are missing".into()))?;
    let revision: i64 = row.try_get("", "revision")?;
    let mut proxies = Vec::new();
    for row in db
        .query_all(stmt(
            "SELECT proxy_id,name,url FROM network_proxy ORDER BY position",
            vec![],
        ))
        .await?
    {
        let id: String = row.try_get("", "proxy_id")?;
        let mut provider_ids = Vec::new();
        for link in db
            .query_all(stmt(
                "SELECT provider_id FROM provider_proxy WHERE proxy_id=? ORDER BY provider_id",
                vec![id.clone().into()],
            ))
            .await?
        {
            provider_ids.push(link.try_get("", "provider_id")?);
        }
        proxies.push(ProxyEntry {
            id,
            name: row.try_get("", "name")?,
            url: row.try_get("", "url")?,
            provider_ids,
            display_url: String::new(),
        });
    }
    Ok(NetworkSettings {
        revision: revision
            .try_into()
            .map_err(|_| DbErr::Custom("Invalid network settings revision".into()))?,
        proxies,
    })
}
pub async fn save(db: &impl ConnectionTrait, settings: &NetworkSettings) -> Result<u64, DbErr> {
    let revision = i64::try_from(settings.revision)
        .ok()
        .filter(|v| *v < i64::MAX)
        .ok_or_else(|| DbErr::Custom("Invalid network settings revision".into()))?;
    let changed = db
        .execute(stmt(
            "UPDATE network_settings SET revision=revision+1 WHERE id=1 AND revision=?",
            vec![revision.into()],
        ))
        .await?;
    if changed.rows_affected() != 1 {
        return Err(DbErr::Custom(
            "Network settings changed; reload before saving".into(),
        ));
    }
    db.execute(stmt("DELETE FROM network_proxy", vec![]))
        .await?;
    for (position, proxy) in settings.proxies.iter().enumerate() {
        db.execute(stmt(
            "INSERT INTO network_proxy(proxy_id,name,url,position) VALUES(?,?,?,?)",
            vec![
                proxy.id.clone().into(),
                proxy.name.clone().into(),
                proxy.url.clone().into(),
                (position as i64).into(),
            ],
        ))
        .await?;
        for id in &proxy.provider_ids {
            db.execute(stmt(
                "INSERT INTO provider_proxy(provider_id,proxy_id) VALUES(?,?)",
                vec![(*id).into(), proxy.id.clone().into()],
            ))
            .await?;
        }
    }
    Ok(settings.revision + 1)
}
