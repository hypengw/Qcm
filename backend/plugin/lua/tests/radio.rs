use qcm_core::{
    db::sync_run::SyncCoordinator,
    provider::{Context, Provider},
};
use qcm_plugin_lua::{plugin::Capabilities, provider::LuaProvider};
use sea_orm::Database;
use std::{collections::HashMap, sync::Arc};

async fn context() -> Context {
    let db = Database::connect("sqlite::memory:").await.unwrap();
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Arc::new(SyncCoordinator::default()),
    }
}

fn provider(functions: &str) -> (LuaProvider, std::path::PathBuf) {
    provider_with_capabilities(functions, Capabilities::default()).unwrap()
}
fn provider_with_capabilities(
    functions: &str,
    capabilities: Capabilities,
) -> qcm_core::Result<(LuaProvider, std::path::PathBuf)> {
    let path = std::env::temp_dir().join(format!("qcm-radio-{}.lua", uuid::Uuid::new_v4()));
    std::fs::write(
        &path,
        format!(
            r#"
local p = {{}}
function p.load() end
function p.save() return '' end
function p.check() end
function p.login() return {{type = 'Ok'}} end
function p.sync() end
function p.sync_item() end
function p.favorite() return true end
function p.image() end
function p.audio() end
function p.subtitle() end
{functions}
return p
"#
        ),
    )
    .unwrap();
    let provider = LuaProvider::with_capabilities(
        Some(1),
        "radio",
        "test-device",
        "radio",
        &path,
        capabilities,
    );
    match provider {
        Ok(provider) => Ok((provider, path)),
        Err(error) => {
            std::fs::remove_file(path).unwrap();
            Err(error)
        }
    }
}

#[tokio::test]
async fn radio_entries_survive_sync_cleanup_and_keep_their_ids() {
    use migration::{Migrator, MigratorTrait};
    use qcm_core::model as m;
    use sea_orm::{ActiveModelTrait, EntityTrait, QueryOrder, Set};
    let ctx = context().await;
    Migrator::up(&ctx.db, Some(4)).await.unwrap();
    migration::MigrationTrait::up(
        &migration::m20261017_000001_provider_session::Migration,
        &migration::SchemaManager::new(&ctx.db),
    )
    .await
    .unwrap();
    m::provider::ActiveModel {
        provider_id: Set(1),
        name: Set("radio".into()),
        type_: Set("test".into()),
        base_url: Set(String::new()),
        auth_method: Set(None),
        cookie: Set(String::new()),
        session_key: Set(String::new()),
        custom: Set(String::new()),
        edit_time: Set(chrono::Utc::now()),
    }
    .insert(&ctx.db)
    .await
    .unwrap();
    let (provider, path) = provider(
        r#"
function p.sync(ctx)
    ctx:sync_radio_queues({{native_id = 'radio/a', name = 'A'}, {native_id = 'radio/b', name = 'B'}})
end
"#,
    );
    let mut previous = None;
    for _ in 0..2 {
        provider.sync(&ctx).await.unwrap();
        let items = m::item::Entity::find()
            .order_by_asc(m::item::Column::Id)
            .all(&ctx.db)
            .await
            .unwrap();
        assert_eq!(items.len(), 2);
        assert_eq!(
            m::remote_mix::Entity::find().all(&ctx.db).await.unwrap().len(),
            2
        );
        assert!(items
            .iter()
            .all(|item| item.r#type == m::type_enum::ItemType::Radio));
        let ids: Vec<_> = items.iter().map(|item| item.id).collect();
        if let Some(ref old) = previous {
            assert_eq!(old, &ids);
        }
        previous = Some(ids);
    }
    std::fs::remove_file(path).unwrap();
}

#[tokio::test]
async fn radio_configuration_and_values_reach_plugin() {
    let (provider, path) = provider(
        r#"
function p.queue_configuration(ctx, native_id)
    assert(native_id == 'radio/test')
    return {{key = 'style', name = 'Style', value = 'calm', options = {
        {value = 'calm', name = 'Calm'},
        {value = 'lively', name = 'Lively', description = 'More energy'},
    }}}
end
function p.get_queue_next(ctx, native_id, current, configuration)
    assert(native_id == 'radio/test' and current[1] == '42' and current[2] == '43')
    assert(configuration.style == 'lively')
    return {}
end
"#,
    );
    let ctx = context().await;
    let fields = provider
        .queue_configuration(&ctx, "radio/test")
        .await
        .unwrap();
    assert_eq!(fields.len(), 1);
    assert_eq!(fields[0].key, "style");
    assert_eq!(fields[0].value, "calm");
    assert_eq!(fields[0].options[1].description, "More energy");
    provider
        .get_queue_next_configured(
            &ctx,
            "radio/test",
            &["42".into(), "43".into()],
            &HashMap::from([("style".into(), "lively".into())]),
        )
        .await
        .unwrap();
    std::fs::remove_file(path).unwrap();
}

#[tokio::test]
async fn radio_configuration_is_optional_for_existing_plugins() {
    let (provider, path) = provider(
        r#"
function p.get_queue_next(ctx, native_id, current)
    assert(native_id == 'radio/legacy' and current[1] == '42')
    return {}
end
"#,
    );
    let ctx = context().await;
    assert!(provider
        .queue_configuration(&ctx, "radio/legacy")
        .await
        .unwrap()
        .is_empty());
    provider
        .get_queue_next(&ctx, "radio/legacy", &["42".into()])
        .await
        .unwrap();
    provider
        .get_queue_next_configured(&ctx, "radio/legacy", &["42".into()], &HashMap::new())
        .await
        .unwrap();
    std::fs::remove_file(path).unwrap();
}

#[tokio::test]
async fn lifecycle_round_trips_private_state_and_presentation() {
    use qcm_core::provider::{QueueInput, QueueNextRequest, QueueOpenReason, QueueOpenRequest};
    let caps = Capabilities {
        requires: vec!["radio.lifecycle.v1".into()],
        ..Default::default()
    };
    let (provider, path) = provider_with_capabilities(r#"
function p.queue_definition(ctx, id)
    return {configuration = {}, can_restart = true, restart_label = 'Again'}
end
function p.open_queue(ctx, id, request)
    assert(request.reason == 'Restart' and request.input.item_type == 'Song')
    assert(request.input.native_id == '42' and request.previous_state == 'old')
    return {songs = {}, state = qcm.json.encode({version = 1, cursor = 'first'}),
        configuration = {style = 'calm'}, presentation = {description = 'From source', source_item_id = 8}}
end
function p.next_queue(ctx, id, request)
    local state = qcm.json.decode(request.state)
    assert(state.version == 1 and state.cursor == 'first')
    assert(request.current_native_ids[1] == '42' and request.current_native_ids[2] == '43')
    return {songs = {}, state = 'finished', configuration = {style = 'calm'}, exhausted = true}
end
function p.close_queue(ctx, id, state) assert(state == 'finished') end
"#, caps).unwrap();
    let ctx = context().await;
    let definition = provider.queue_definition(&ctx, "test").await.unwrap();
    assert!(definition.can_restart);
    assert_eq!(definition.restart_label, "Again");
    let batch = provider
        .open_queue(
            &ctx,
            "test",
            QueueOpenRequest {
                reason: QueueOpenReason::Restart,
                input: Some(QueueInput {
                    native_id: "42".into(),
                    item_type: qcm_core::model::type_enum::ItemType::Song,
                }),
                previous_state: Some("old".into()),
                ..Default::default()
            },
        )
        .await
        .unwrap();
    assert_eq!(batch.presentation.source_item_id, Some(8));
    assert_eq!(batch.presentation.description, "From source");
    let batch = provider
        .next_queue(
            &ctx,
            "test",
            QueueNextRequest {
                state: batch.state,
                current_native_ids: vec!["42".into(), "43".into()],
            },
        )
        .await
        .unwrap();
    assert!(batch.exhausted);
    provider
        .close_queue(&ctx, "test", &batch.state)
        .await
        .unwrap();
    let epoch = provider.lifecycle_epoch();
    provider.invalidate();
    assert!(provider.lifecycle_epoch() > epoch);
    std::fs::remove_file(path).unwrap();
}

#[tokio::test]
async fn legacy_lifecycle_preserves_configuration_and_current_order() {
    use qcm_core::provider::{QueueNextRequest, QueueOpenRequest};
    let (provider, path) = provider(
        r#"
function p.queue_configuration() return {{key = 'style', name = 'Style', value = 'calm', options = {}}} end
function p.get_queue_next(ctx, id, current, configuration)
    assert(configuration.style == 'calm')
    if #current > 0 then assert(current[1] == 'b' and current[2] == 'a') end
    return {}
end
"#,
    );
    let ctx = context().await;
    assert!(
        !provider
            .queue_definition(&ctx, "legacy")
            .await
            .unwrap()
            .can_restart
    );
    let batch = provider
        .open_queue(
            &ctx,
            "legacy",
            QueueOpenRequest {
                configuration: HashMap::from([("style".into(), "calm".into())]),
                ..Default::default()
            },
        )
        .await
        .unwrap();
    let next = provider
        .next_queue(
            &ctx,
            "legacy",
            QueueNextRequest {
                state: batch.state,
                current_native_ids: vec!["b".into(), "a".into()],
            },
        )
        .await
        .unwrap();
    assert_eq!(next.configuration["style"], "calm");
    provider
        .close_queue(&ctx, "legacy", &next.state)
        .await
        .unwrap();
    std::fs::remove_file(path).unwrap();
}

#[test]
fn lifecycle_hooks_and_capability_are_required_together() {
    let caps = || Capabilities {
        requires: vec!["radio.lifecycle.v1".into()],
        ..Default::default()
    };
    assert!(provider_with_capabilities("function p.open_queue() end", caps()).is_err());
    assert!(provider_with_capabilities("", caps()).is_err());
    assert!(
        provider_with_capabilities("function p.close_queue() end", Default::default()).is_err()
    );
    assert!(provider_with_capabilities(
        r#"
function p.queue_definition() end
function p.open_queue() end
function p.next_queue() end
"#,
        Default::default()
    )
    .is_err());
}

#[tokio::test]
async fn lifecycle_optional_input_and_cleanup_remain_optional() {
    use qcm_core::provider::QueueOpenRequest;
    let caps = Capabilities {
        requires: vec!["radio.lifecycle.v1".into()],
        ..Default::default()
    };
    let (provider, path) = provider_with_capabilities(
        r#"
function p.queue_definition() return {} end
function p.open_queue(ctx, id, request)
    assert(request.input == nil and request.previous_state == nil and request.reason == 'Initial')
    assert(type(request.configuration) == 'table')
    return {songs = {}, state = 'opaque', presentation = {}}
end
function p.next_queue() return {songs = {}} end
"#,
        caps,
    )
    .unwrap();
    let ctx = context().await;
    let batch = provider
        .open_queue(&ctx, "test", QueueOpenRequest::default())
        .await
        .unwrap();
    assert!(batch.presentation.source_item_id.is_none());
    provider
        .close_queue(&ctx, "test", &batch.state)
        .await
        .unwrap();
    std::fs::remove_file(path).unwrap();
}
