use sea_orm::{ConnectionTrait, DbErr};

pub fn create_fts_table_sql(table_name: &str, columns: &[&str]) -> String {
    let columns_str = columns.join(",");
    format!(
        r#"CREATE VIRTUAL TABLE IF NOT EXISTS {table_name}_fts USING fts5 (
                {columns_str},
                content='{table_name}',
                content_rowid='id',
                tokenize='qcm'
            );"#,
    )
}

pub async fn create_fts_triggers<C>(db: &C, table_name: &str, columns: &[&str]) -> Result<(), DbErr>
where
    C: ConnectionTrait,
{
    // Create triggers
    db.execute_unprepared(&create_insert_trigger_sql(table_name, columns))
        .await?;
    db.execute_unprepared(&create_delete_trigger_sql(table_name, columns))
        .await?;
    db.execute_unprepared(&create_update_trigger_sql(table_name, columns))
        .await?;
    Ok(())
}

pub async fn drop_fts_triggers<C>(db: &C, table_name: &str) -> Result<(), DbErr>
where
    C: ConnectionTrait,
{
    // Drop triggers
    db.execute_unprepared(&format!("DROP TRIGGER IF EXISTS {}_fts_i;", table_name))
        .await?;
    db.execute_unprepared(&format!("DROP TRIGGER IF EXISTS {}_fts_d;", table_name))
        .await?;
    db.execute_unprepared(&format!("DROP TRIGGER IF EXISTS {}_fts_u;", table_name))
        .await?;
    Ok(())
}

pub async fn create_fts_table_and_triggers<C>(
    db: &C,
    table_name: &str,
    columns: &[&str],
) -> Result<(), DbErr>
where
    C: ConnectionTrait,
{
    db.execute_unprepared(&create_fts_table_sql(table_name, columns))
        .await?;
    create_fts_triggers(db, table_name, columns).await?;
    Ok(())
}

fn create_insert_trigger_sql(table_name: &str, columns: &[&str]) -> String {
    let columns_str = columns.join(", ");
    let values_str = columns
        .iter()
        .map(|col| format!("new.{}", col))
        .collect::<Vec<_>>()
        .join(", ");
    format!(
        r#"CREATE TRIGGER {table_name}_fts_i AFTER INSERT ON {table_name} BEGIN
                INSERT INTO {table_name}_fts(rowid, {columns_str}) VALUES (new.id, {values_str});
            END;"#,
    )
}

fn create_delete_trigger_sql(table_name: &str, columns: &[&str]) -> String {
    let columns_str = columns.join(", ");
    let values_str = columns
        .iter()
        .map(|col| format!("old.{}", col))
        .collect::<Vec<_>>()
        .join(", ");
    format!(
        r#"CREATE TRIGGER {table_name}_fts_d AFTER DELETE ON {table_name} BEGIN
                INSERT INTO {table_name}_fts({table_name}_fts, rowid, {columns_str}) VALUES('delete', old.id, {values_str});
            END;"#,
    )
}

fn create_update_trigger_sql(table_name: &str, columns: &[&str]) -> String {
    let columns_str = columns.join(", ");
    let old_values_str = columns
        .iter()
        .map(|col| format!("old.{}", col))
        .collect::<Vec<_>>()
        .join(", ");
    let new_values_str = columns
        .iter()
        .map(|col| format!("new.{}", col))
        .collect::<Vec<_>>()
        .join(", ");
    format!(
        r#"CREATE TRIGGER {table_name}_fts_u AFTER UPDATE ON {table_name} BEGIN
                INSERT INTO {table_name}_fts({table_name}_fts, rowid, {columns_str}) VALUES('delete', old.id, {old_values_str});
                INSERT INTO {table_name}_fts(rowid, {columns_str}) VALUES (new.id, {new_values_str});
            END;"#
    )
}
