use qcm_core::{
    error::ProviderError,
    provider::{CardAction, Context, Provider},
};
use qcm_plugin_lua::provider::LuaProvider;

async fn context() -> Context {
    let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Default::default(),
    }
}
fn provider(callback: &str) -> (LuaProvider, std::path::PathBuf) {
    let path = std::env::temp_dir().join(format!("qcm-home-{}.lua", uuid::Uuid::new_v4()));
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
function p.favorite() end
function p.image() end
function p.audio() end
function p.subtitle() end
{callback}
return p
"#
        ),
    )
    .unwrap();
    (
        LuaProvider::new(Some(1), "home", "test", "home", &path).unwrap(),
        path,
    )
}
#[tokio::test]
async fn home_optional_and_typed_metadata() {
    let ctx = context().await;
    let (old, path) = provider("");
    assert!(matches!(
        old.for_you_items(&ctx).await,
        Err(ProviderError::NotImplemented)
    ));
    std::fs::remove_file(path).unwrap();
    let (empty, path) = provider("function p.for_you_items() return {} end");
    assert!(empty.for_you_items(&ctx).await.unwrap().is_empty());
    std::fs::remove_file(path).unwrap();
    let (typed, path) = provider(
        r#"
function p.for_you_items()
    return {{entry_key = 'radio', target = {native_id = 'radio/a', library_id = qcm.null, type = 'Radio'},
        presentation = {style = 'Radio', cover = {type = 'RadioCurrent'}},
        primary_action = 'ActivateRadio', play_action = 'ActivateRadio'}}
end
"#,
    );
    let entries = typed.for_you_items(&ctx).await.unwrap();
    assert_eq!(entries.len(), 1);
    assert_eq!(entries[0].primary_action, CardAction::ActivateRadio);
    std::fs::remove_file(path).unwrap();
    for callback in [
        "function p.for_you_items() return {{entry_key = 'bad', target = 1, primary_action = 'arbitraryScript'}} end",
        "function p.for_you_items() return {{entry_key = 'bad', target = 1, primary_action = 'PlayItem', route = 'javascript'}} end",
    ] {
        let (invalid, path) = provider(callback);
        assert!(invalid.for_you_items(&ctx).await.is_err());
        std::fs::remove_file(path).unwrap();
    }
}
