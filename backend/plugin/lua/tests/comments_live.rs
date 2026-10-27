use qcm_core::{
    model::type_enum::ItemType,
    provider::{CommentOrder, CommentPageRequest, CommentTarget, Context, Provider},
};
use qcm_plugin_lua::provider::LuaProvider;

#[tokio::test]
#[ignore = "public network read; requires QCM_NCM_SOURCE"]
async fn ncm_public_comments_use_the_real_client() {
    let source =
        std::env::var("QCM_NCM_SOURCE").expect("set QCM_NCM_SOURCE to the ncm src directory");
    let path =
        std::env::temp_dir().join(format!("qcm-public-comments-{}.lua", uuid::Uuid::new_v4()));
    std::fs::write(&path, format!(r#"
package.path = {source:?} .. '/?.lua;' .. package.path
local client = require('client').new('qcm-comments-read-test')
local comments = require('comments')
local p = {{}}
function p.load() end
function p.save() return '' end
function p.check() error('anonymous read must not log in') end
function p.login() error('anonymous read must not log in') end
function p.sync() end
function p.sync_item() end
function p.favorite() error('no writes') end
function p.image() end
function p.audio() end
function p.subtitle() end
function p.comments(ctx, target, request) return comments.list(client, target, request) end
function p.comment_replies(ctx, target, parent, request) return comments.list(client, target, request, parent) end
return p
"#)).unwrap();
    let provider = LuaProvider::new(None, "public comments", "ncm", "test", &path).unwrap();
    let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    let ctx = Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Default::default(),
    };
    let target = CommentTarget {
        native_id: "186016".into(),
        item_type: ItemType::Song,
    };
    for order in [CommentOrder::Hot, CommentOrder::Latest] {
        let mut request = CommentPageRequest {
            order,
            state: None,
            limit: 2,
        };
        let first = provider
            .comments(&ctx, target.clone(), request.clone())
            .await
            .unwrap()
            .unwrap();
        assert_eq!(first.comments.len(), 2);
        assert!(first.total.is_some() && first.has_more);
        request.state = first.next_state;
        let next = provider
            .comments(&ctx, target.clone(), request)
            .await
            .unwrap()
            .unwrap();
        assert_eq!(next.comments.len(), 2);
    }
    let page = provider
        .comment_replies(
            &ctx,
            target,
            "5257496022",
            CommentPageRequest {
                order: CommentOrder::Latest,
                state: None,
                limit: 2,
            },
        )
        .await
        .unwrap()
        .unwrap();
    assert!(page.total.is_some());
    std::fs::remove_file(path).unwrap();
}
