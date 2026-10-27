use super::*;
use prost::Message;
use qcm_core::{model as m, model::type_enum::ItemType, provider::Provider};
use qcm_plugin_lua::provider::LuaProvider;
use sea_orm::{ConnectionTrait, Database, Schema};

fn provider(id: i64, callback: bool) -> (Arc<LuaProvider>, std::path::PathBuf) {
    let path = std::env::temp_dir().join(format!("qcm-comments-{}.lua", uuid::Uuid::new_v4()));
    let functions = if callback {
        r#"
function p.comments(ctx, target, request)
    assert(target.native_id == 'remote' and target.item_type == 'Mix')
    if request.limit == 1 then qcm.inner:begin_session() end
    if request.state then assert(request.state == 'next') end
    return {comments={{id='9007199254740993',content='text',liked=false,like_count=0}},
        order=request.order,orders={'Hot','Latest'},total=1,has_more=not request.state,
        next_state=not request.state and 'next' or nil,can_reply=true,can_like=true}
end
function p.comment_replies(ctx, target, parent, request)
    assert(parent == '9007199254740993')
    return {comments={},order=request.order,orders={'Latest'},total=0,has_more=false,can_reply=false,can_like=true}
end
function p.set_comment_like(ctx, target, id, value)
    assert(id == '9007199254740993' and target.native_id == 'remote')
    return {liked=value}
end
"#
    } else {
        ""
    };
    std::fs::write(
        &path,
        format!(
            r#"
local p = {{}}
function p.load() end
function p.save() return '' end
function p.check() error('unexpected authentication') end
function p.login() end
function p.sync() error('unexpected sync') end
function p.sync_item() error('unexpected sync') end
function p.favorite() error('not a media favorite') end
function p.image() end
function p.audio() error('unexpected audio') end
function p.subtitle() end
{functions}
return p
"#
        ),
    )
    .unwrap();
    (
        Arc::new(LuaProvider::new(Some(id), "comments", "test", "test", &path).unwrap()),
        path,
    )
}

#[tokio::test]
async fn comments_round_trip_paging_reactions_and_provider_replacement() {
    const ID: i64 = 819206;
    let db = Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=OFF")
        .await
        .unwrap();
    let schema = Schema::new(db.get_database_backend());
    for table in [
        schema.create_table_from_entity(m::item::Entity),
        schema.create_table_from_entity(m::mix::Entity),
    ] {
        db.execute(db.get_database_backend().build(&table))
            .await
            .unwrap();
    }
    db.execute_unprepared(&format!("INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at) VALUES (501,'remote',{ID},54,0,0,0);
        INSERT INTO mix(id,name,description,track_count,mix_type,remote_id) VALUES(91,'Remote','',0,2,501),(92,'Local','',0,0,NULL);")).await.unwrap();
    db.execute_unprepared(&format!("INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at) VALUES(502,'other',{ID},54,0,0,0); INSERT INTO mix(id,name,description,track_count,mix_type,remote_id) VALUES(93,'Other','',0,2,502);")).await.unwrap();
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    let ctx = Arc::new(Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Default::default(),
    });
    let (plugin, path) = provider(ID, true);
    global::add_provider(plugin.clone());
    let _cleanup = scopeguard::guard((ID, path), |(id, path)| {
        global::remove_provider(id);
        let _ = std::fs::remove_file(path);
    });
    let mut req = msg::GetCommentsReq {
        id: 91,
        item_type: ItemType::Mix as i32,
        order: 0,
        limit: 2,
        token: None,
    };
    let first = list(&ctx, &req, None, None).await.unwrap();
    assert!(first.supported);
    let page = first.page.as_ref().unwrap();
    assert_eq!(page.comments[0].liked, Some(false));
    assert_eq!(page.comments[0].like_count, Some(0));
    assert_eq!(page.comments[0].id, "9007199254740993");
    req.token = page.next_token.clone();
    assert!(
        !list(&ctx, &req, None, None)
            .await
            .unwrap()
            .page
            .unwrap()
            .has_more
    );
    req.order = 1;
    assert!(list(&ctx, &req, None, None).await.is_err());
    req.order = 0;
    assert!(list(&ctx, &req, Some("other"), Some(&first.scope))
        .await
        .is_err());
    req.token = None;
    assert_eq!(
        list(&ctx, &req, Some("9007199254740993"), Some(&first.scope))
            .await
            .unwrap()
            .page
            .unwrap()
            .total,
        Some(0)
    );
    let mut reaction = msg::SetCommentLikeReq {
        id: 91,
        item_type: req.item_type,
        comment_id: "9007199254740993".into(),
        value: true,
        scope: first.scope.clone(),
    };
    assert!(like(&ctx, &reaction).await.unwrap().result.unwrap().liked);
    reaction.value = false;
    assert!(!like(&ctx, &reaction).await.unwrap().result.unwrap().liked);
    req.id = 92;
    assert!(!list(&ctx, &req, None, None).await.unwrap().supported);
    req.id = 91;
    req.item_type = ItemType::Song as i32;
    assert!(list(&ctx, &req, None, None).await.is_err());
    req.item_type = ItemType::Mix as i32;
    let directory =
        std::env::temp_dir().join(format!("qcm-comments-cache-{}", uuid::Uuid::new_v4()));
    let cache = crate::resource_cache::ResourceCache::open(directory.clone())
        .await
        .unwrap();
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let (oper, _) = crate::task::TaskManager::new();
    let service = Arc::new(crate::event::ServiceContext {
        provider_context: ctx.clone(),
        backend_ev,
        oper,
        mix_refresh: Default::default(),
        resource_cache: cache.clone(),
        radio: Default::default(),
        playback: Default::default(),
    });
    for (kind, payload) in [
        (
            msg::MessageType::GetCommentsReq,
            msg::qcm_message::Payload::GetCommentsReq(req.clone()),
        ),
        (
            msg::MessageType::GetCommentRepliesReq,
            msg::qcm_message::Payload::GetCommentRepliesReq(msg::GetCommentRepliesReq {
                query: Some(req.clone()),
                parent_id: reaction.comment_id.clone(),
                scope: first.scope.clone(),
            }),
        ),
        (
            msg::MessageType::SetCommentLikeReq,
            msg::qcm_message::Payload::SetCommentLikeReq(reaction.clone()),
        ),
    ] {
        let wire = msg::QcmMessage {
            r#type: kind as i32,
            payload: Some(payload),
            ..Default::default()
        };
        let reply = crate::api::process_qcm(&service, &wire.encode_to_vec(), &mut None)
            .await
            .unwrap();
        assert!(reply.payload.is_some());
        msg::QcmMessage::decode(reply.encode_to_vec().as_slice()).unwrap();
    }
    reaction.id = 93;
    assert!(matches!(
        like(&ctx, &reaction).await,
        Err(ProcessError::NotAuth)
    ));
    reaction.id = 91;
    req.limit = 1;
    let changed = list(&ctx, &req, None, None).await;
    assert!(matches!(changed, Err(ProcessError::NotAuth)), "{changed:?}");
    req.limit = 2;
    plugin.invalidate();
    assert!(matches!(
        like(&ctx, &reaction).await,
        Err(ProcessError::NotAuth)
    ));
    let (replacement, path) = provider(ID, false);
    global::add_provider(replacement);
    assert!(!list(&ctx, &req, None, None).await.unwrap().supported);
    assert!(matches!(
        like(&ctx, &reaction).await,
        Err(ProcessError::NotAuth)
    ));
    std::fs::remove_file(path).unwrap();
    cache.shutdown().await.unwrap();
    let _ = std::fs::remove_dir_all(directory);
}
