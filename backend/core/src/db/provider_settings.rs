use crate::{
    model::provider_settings as m,
    provider::settings::{SettingValues, SettingsSnapshot},
};
use sea_orm::{sea_query::Expr, *};

pub async fn restore(
    db: &impl ConnectionTrait,
    provider: &dyn crate::provider::Provider,
) -> crate::Result<()> {
    let mut saved = load(
        db,
        provider
            .id()
            .ok_or_else(|| crate::anyhow!("Provider has no ID"))?,
    )
    .await?;
    saved.values = provider.validate_settings(&saved.values)?;
    provider.set_settings(saved);
    Ok(())
}

pub async fn load(db: &impl ConnectionTrait, id: i64) -> Result<SettingsSnapshot, DbErr> {
    match m::Entity::find_by_id(id).one(db).await? {
        Some(row) => Ok(SettingsSnapshot {
            revision: row
                .revision
                .try_into()
                .map_err(|_| DbErr::Custom("Invalid settings revision".into()))?,
            values: serde_json::from_value(row.config).map_err(|e| DbErr::Custom(e.to_string()))?,
        }),
        None => Ok(SettingsSnapshot::default()),
    }
}

pub async fn save(
    db: &DatabaseTransaction,
    id: i64,
    revision: u64,
    values: SettingValues,
) -> Result<SettingsSnapshot, DbErr> {
    let old =
        i64::try_from(revision).map_err(|_| DbErr::Custom("Invalid settings revision".into()))?;
    let next = old
        .checked_add(1)
        .ok_or_else(|| DbErr::Custom("Settings revision exhausted".into()))?;
    db.execute(Statement::from_sql_and_values(DbBackend::Sqlite,
        "INSERT INTO provider_settings(provider_id,revision,config) VALUES (?,0,'{}') ON CONFLICT(provider_id) DO NOTHING", [id.into()])).await?;
    let result = m::Entity::update_many()
        .col_expr(m::Column::Revision, Expr::value(next))
        .col_expr(
            m::Column::Config,
            Expr::value(serde_json::to_value(&values).map_err(|e| DbErr::Custom(e.to_string()))?),
        )
        .filter(m::Column::ProviderId.eq(id))
        .filter(m::Column::Revision.eq(old))
        .exec(db)
        .await?;
    if result.rows_affected != 1 {
        return Err(DbErr::Custom(
            "Provider settings changed; reload before saving".into(),
        ));
    }
    Ok(SettingsSnapshot {
        revision: next as u64,
        values,
    })
}
