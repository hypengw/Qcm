pub mod filter;

use qcm_core::model as sqlm;
use qcm_core::provider::Provider;
use sea_orm::*;
use sea_orm::{sea_query, DatabaseConnection, EntityTrait};
use std::sync::Arc;

use crate::error::ProcessError;

pub async fn add_provider(
    db: &DatabaseConnection,
    p: Arc<dyn Provider>,
) -> Result<i64, ProcessError> {
    let id = write_provider(db, &p).await?;
    p.set_id(Some(id));
    p.bind_session(db.clone(), p.session_key().unwrap_or_default());
    Ok(id)
}

pub async fn write_provider(db: &impl ConnectionTrait, p: &Arc<dyn Provider>) -> Result<i64, ProcessError> {
    let model = sqlm::provider::ActiveModel {
        provider_id: match p.id() {
            Some(id) => Set(id),
            None => NotSet,
        },
        name: Set(p.name()),
        type_: Set(p.type_name().to_string()),
        base_url: Set(p.base_url()),
        auth_method: Set(p.auth_method().and_then(|a| serde_json::to_value(a).ok())),
        cookie: Set(p.save_cookie()),
        session_key: Set(p.session_key().unwrap_or_default()),
        custom: Set(p.save()),
        edit_time: Set(chrono::Utc::now()),
    };

    let r = sqlm::provider::Entity::insert(model)
        .on_conflict(
            sea_query::OnConflict::column(sqlm::provider::Column::ProviderId)
                .update_columns([
                    sqlm::provider::Column::Name,
                    sqlm::provider::Column::BaseUrl,
                    sqlm::provider::Column::Custom,
                    sqlm::provider::Column::AuthMethod,
                    sqlm::provider::Column::Cookie,
                    sqlm::provider::Column::SessionKey,
                    sqlm::provider::Column::EditTime,
                ])
                .to_owned(),
        )
        .exec(db)
        .await?;
    Ok(r.last_insert_id)
}

pub async fn update_provider_name(db: &impl ConnectionTrait, id: i64, session: &str, name: &str) -> Result<(), ProcessError> {
    use sea_orm::sea_query::Expr;
    let result = sqlm::provider::Entity::update_many()
        .col_expr(sqlm::provider::Column::Name, Expr::value(name))
        .filter(sqlm::provider::Column::ProviderId.eq(id))
        .filter(sqlm::provider::Column::SessionKey.eq(session))
        .exec(db)
        .await?;
    if result.rows_affected != 1 {
        return Err(ProcessError::NotAuth);
    }
    Ok(())
}
