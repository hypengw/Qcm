use crate::{error::ProcessError, msg};
use qcm_core::network::{self, NetworkSettings, ProxyEntry};
use sea_orm::{DatabaseConnection, TransactionTrait};
use tokio::sync::Mutex;
static EDIT: Mutex<()> = Mutex::const_new(());

fn invalid(message: impl Into<String>) -> ProcessError {
    anyhow::anyhow!(message.into()).into()
}

pub async fn restore(db: &DatabaseConnection) -> Result<(), ProcessError> {
    let _edit = EDIT.lock().await;
    let txn = db.begin().await?;
    let settings = qcm_core::db::network::load(&txn).await?;
    let compiled = network::compile(settings).map_err(invalid)?;
    txn.commit().await?;
    network::publish(compiled);
    Ok(())
}
pub async fn read(db: &DatabaseConnection) -> Result<msg::GetNetworkSettingsRsp, ProcessError> {
    let _edit = EDIT.lock().await;
    let txn = db.begin().await?;
    let compiled = network::compile(qcm_core::db::network::load(&txn).await?).map_err(invalid)?;
    txn.commit().await?;
    Ok(msg::GetNetworkSettingsRsp {
        proxies_json: serde_json::to_string(&compiled.settings.proxies)
            .map_err(anyhow::Error::from)?,
        revision: compiled.settings.revision,
    })
}
pub async fn update(
    db: &DatabaseConnection,
    req: &msg::UpdateNetworkSettingsReq,
) -> Result<msg::Rsp, ProcessError> {
    if req.proxies_json.len() > 512 * 1024 {
        return Err(invalid("Proxy settings are too large"));
    }
    let proxies: Vec<ProxyEntry> =
        serde_json::from_str(&req.proxies_json).map_err(|_| invalid("Invalid proxy list"))?;
    let compiled = network::compile(NetworkSettings {
        revision: req.revision,
        proxies,
    })
    .map_err(invalid)?;
    let _edit = EDIT.lock().await;
    let txn = db.begin().await?;
    use sea_orm::EntityTrait;
    for proxy in &compiled.settings.proxies {
        for id in &proxy.provider_ids {
            if qcm_core::model::provider::Entity::find_by_id(*id)
                .one(&txn)
                .await?
                .is_none()
            {
                return Err(invalid(
                    "A selected provider no longer exists; reload before saving",
                ));
            }
        }
    }
    qcm_core::db::network::save(&txn, &compiled.settings).await?;
    txn.commit().await?;
    network::publish(compiled);
    Ok(msg::Rsp::default())
}

#[cfg(test)]
#[path = "network_tests.rs"]
mod tests;
