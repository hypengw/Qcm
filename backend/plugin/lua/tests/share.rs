use qcm_core::{model::type_enum::ItemType, provider::Provider};
use qcm_plugin_lua::provider::LuaProvider;

fn provider(callback: &str) -> (LuaProvider, std::path::PathBuf) {
    let path = std::env::temp_dir().join(format!("qcm-share-{}.lua", uuid::Uuid::new_v4()));
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
        LuaProvider::new(Some(1), "share", "test", "share", &path).unwrap(),
        path,
    )
}

#[test]
fn share_url_is_optional_and_preserves_native_ids_and_types() {
    let (old, path) = provider("");
    assert!(old.share_url("1", ItemType::Song).unwrap().is_none());
    std::fs::remove_file(path).unwrap();
    let (plugin, path) = provider(
        r#"
function p.share_url(id, kind)
    if kind ~= 101 then return nil end
    assert(id == '9007199254740993')
    return 'https://music.163.com/song?id=' .. id
end
"#,
    );
    assert_eq!(
        plugin
            .share_url("9007199254740993", ItemType::Song)
            .unwrap()
            .unwrap()
            .as_str(),
        "https://music.163.com/song?id=9007199254740993"
    );
    assert!(plugin.share_url("1", ItemType::Album).unwrap().is_none());
    std::fs::remove_file(path).unwrap();
}

#[test]
fn invalid_share_url_is_not_returned_as_a_valid_result() {
    for callback in [
        "function p.share_url() return 'not a URL' end",
        "function p.share_url() error('failed') end",
    ] {
        let (plugin, path) = provider(callback);
        assert!(plugin.share_url("1", ItemType::Song).is_err());
        std::fs::remove_file(path).unwrap();
    }
}
