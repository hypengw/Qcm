use crate::{
    convert::QcmInto,
    error::ProcessError,
    event::{BackendEvent, ServiceContext},
    msg,
};
use qcm_core::{
    global,
    provider::{
        settings::{SettingValues, SettingsError, SettingsSnapshot},
        AuthResult, Provider,
    },
};
use sea_orm::TransactionTrait;
use std::sync::Arc;
#[cfg(test)]
#[path = "provider_settings_tests.rs"]
mod tests;

pub fn read(id: i64) -> Result<msg::GetProviderSettingsRsp, ProcessError> {
    let provider = global::provider(id).ok_or(ProcessError::NoSuchProvider(id.to_string()))?;
    let snapshot = provider.settings();
    Ok(msg::GetProviderSettingsRsp {
        definition_json: serde_json::to_string(&provider.settings_definition()?)
            .map_err(anyhow::Error::from)?,
        values_json: serde_json::to_string(&snapshot.values).map_err(anyhow::Error::from)?,
        revision: snapshot.revision,
    })
}

fn invalid(error: SettingsError) -> msg::UpdateProviderRsp {
    msg::UpdateProviderRsp {
        code: msg::model::AuthResult::Failed as i32,
        message: error.message,
        field_errors: error.fields.into_iter().collect(),
    }
}

pub async fn update(
    ctx: &Arc<ServiceContext>,
    req: &msg::UpdateProviderReq,
) -> Result<msg::UpdateProviderRsp, ProcessError> {
    let old = global::provider(req.provider_id)
        .ok_or(ProcessError::NoSuchProvider(req.provider_id.to_string()))?;
    if req.auth_info.is_some() && !req.tmp_provider.is_empty() {
        return Ok(invalid(SettingsError::message(
            "Choose one authentication update",
        )));
    }
    let candidate: Arc<dyn Provider> = if !req.tmp_provider.is_empty() {
        let provider = global::get_tmp_provider(&req.tmp_provider)
            .ok_or(ProcessError::NoSuchProvider(req.tmp_provider.clone()))?;
        if provider.type_name() != old.type_name() {
            return Err(ProcessError::NotAuth);
        }
        provider
    } else if req.auth_info.is_some() {
        let meta = global::provider_meta(&old.type_name())
            .ok_or(ProcessError::NoSuchProviderType(old.type_name()))?;
        (meta.creator)(old.id(), &req.name, &global::device_id())?
    } else {
        old.clone()
    };
    let values: SettingValues = match serde_json::from_str(&req.settings_json) {
        Ok(values) => values,
        Err(_) => {
            return Ok(invalid(SettingsError::message(
                "Settings must be a JSON object",
            )))
        }
    };
    let values = match candidate.validate_settings(&values) {
        Ok(values) => values,
        Err(error) => return Ok(invalid(error)),
    };
    let replacing = !Arc::ptr_eq(&old, &candidate);
    if replacing {
        candidate.set_settings(SettingsSnapshot {
            revision: req.settings_revision,
            values: values.clone(),
        });
    }
    if let Some(info) = &req.auth_info {
        match candidate
            .auth(&ctx.provider_context, &info.clone().qcm_into())
            .await?
        {
            AuthResult::Ok => (),
            result => return Ok(result.qcm_into()),
        }
    }
    let write = ctx
        .provider_context
        .sync_coordinator
        .write(req.provider_id, None)
        .await?;
    if global::provider(req.provider_id).is_none_or(|current| !Arc::ptr_eq(&old, &current)) {
        return Err(ProcessError::NotAuth);
    }
    let db = &ctx.provider_context.db;
    let txn = db.begin().await?;
    let snapshot =
        qcm_core::db::provider_settings::save(&txn, req.provider_id, req.settings_revision, values)
            .await?;
    if replacing {
        candidate.set_id(Some(req.provider_id));
        candidate.set_name(&req.name);
        crate::db::write_provider(&txn, &candidate).await?;
    } else {
        crate::db::update_provider_name(
            &txn,
            req.provider_id,
            &old.session_key().unwrap_or_default(),
            &req.name,
        )
        .await?;
    }
    txn.commit().await?;
    candidate.set_name(&req.name);
    candidate.set_settings(snapshot);
    if replacing {
        write.invalidate_sync();
        candidate.bind_session(db.clone(), candidate.session_key().unwrap_or_default());
        ctx.playback.invalidate(req.provider_id).await;
        global::add_provider(candidate);
        ctx.radio
            .invalidate(&ctx.provider_context, req.provider_id)
            .await;
        if let Err(error) = ctx.resource_cache.invalidate(req.provider_id).await {
            tracing::warn!(provider_id = req.provider_id, %error, "Provider saved but cache invalidation failed");
        }
    }
    drop(write);
    let _ = ctx
        .backend_ev
        .send(BackendEvent::UpdateProvider {
            id: req.provider_id,
        })
        .await;
    Ok(msg::UpdateProviderRsp {
        code: msg::model::AuthResult::Ok as i32,
        ..Default::default()
    })
}
