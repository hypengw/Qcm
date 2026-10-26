use super::*;
use migration::{Migrator, MigratorTrait};
use qcm_core::{
    db::{sync_batch::ProviderWriter, sync_run::SyncScope},
    model as m,
    provider::ProviderCommon,
};
use sea_orm::*;
use serde_json::json;

const PROVIDER: i64 = 872148;

#[tokio::test]
async fn rolling_queue_retention_stays_bounded_across_many_batches() {
    let (owner, ctx, target, provider, library, ids) = fixture().await;
    *provider.songs.lock().unwrap() = vec![ids[0]];
    let mut page = open(&owner, &ctx, &target).await;
    let album = m::song::Entity::find_by_id(ids[0])
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap()
        .album_id
        .unwrap();
    let mut queued = vec!["private".to_owned()];
    for round in 0..64 {
        let songs: Vec<_> = (0..6).map(|index| json!({
            "item":{"library_id":library,"type":"Song","native_id":format!("batch-{round}-{index}")},
            "name":"Song","album_id":album,"duration":180000,"can_play":true,
        })).collect();
        let result = ProviderWriter::new(&ctx.db, PROVIDER, &ctx.sync_coordinator)
            .sync_batch(serde_json::from_value(json!({"songs":songs})).unwrap())
            .await
            .unwrap();
        *provider.songs.lock().unwrap() = result.songs.iter().map(|song| song.id).collect();
        page = owner
            .next(
                ctx.clone(),
                target.clone(),
                page.session_id.clone(),
                operation_id(),
                page.version,
                queued,
            )
            .await
            .unwrap();
        prune(&ctx, library, true).await;
        let write = ctx.sync_coordinator.write(PROVIDER, None).await.unwrap();
        assert!(write.retained_items().len() <= 10);
        drop(write);
        assert!(m::song::Entity::find().count(&ctx.db).await.unwrap() <= 8);
        queued = vec![format!("batch-{round}-4"), format!("batch-{round}-5")];
    }
    owner.close(&ctx, &page.session_id).await;
    prune(&ctx, library, true).await;
    assert_eq!(m::item::Entity::find().count(&ctx.db).await.unwrap(), 0);
}

async fn fixture() -> (
    Arc<RadioSessions>,
    Arc<Context>,
    Target,
    Arc<TestProvider>,
    i64,
    Vec<i64>,
) {
    let db = Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=ON")
        .await
        .unwrap();
    Migrator::up(&db, Some(4)).await.unwrap();
    for migration in [
        &migration::m20261007_000001_image_type_integer::Migration
            as &dyn migration::MigrationTrait,
        &migration::m20261017_000001_provider_session::Migration,
    ] {
        migration
            .up(&migration::SchemaManager::new(&db))
            .await
            .unwrap();
    }
    m::provider::ActiveModel {
        provider_id: Set(PROVIDER),
        name: Set("test".into()),
        type_: Set("test".into()),
        base_url: Set(String::new()),
        auth_method: Set(None),
        cookie: Set(String::new()),
        session_key: Set(String::new()),
        custom: Set(String::new()),
        edit_time: Set(chrono::Utc::now()),
    }
    .insert(&db)
    .await
    .unwrap();
    let (ev_sender, _) = tokio::sync::mpsc::channel(1);
    let ctx = Arc::new(Context {
        db: db.clone(),
        cache_db: db,
        ev_sender,
        sync_coordinator: Default::default(),
    });
    let writer = ProviderWriter::new(&ctx.db, PROVIDER, &ctx.sync_coordinator);
    let library = writer
        .sync_batch(
            serde_json::from_value(json!({
                "libraries":[{"native_id":"account","name":"Library"}],
            }))
            .unwrap(),
        )
        .await
        .unwrap()
        .libraries[0]
        .library_id;
    let key =
        |kind: &str, native: &str| json!({"library_id":library,"type":kind,"native_id":native});
    let album = key("Album", "album");
    let artist = key("Artist", "artist");
    let natives = ["private", "similar", "seed", "next", "stale"];
    let songs: Vec<_> = natives.iter().map(|native| json!({
        "item":key("Song",native),"name":native,"duration":180000,"album_id":album,"can_play":true,
    })).collect();
    let relations: Vec<_> = natives
        .iter()
        .map(|native| {
            json!({
                "owner":key("Song",native),"mode":"replace","targets":[artist],
            })
        })
        .collect();
    let result = writer
        .sync_batch(
            serde_json::from_value(json!({
                "albums":[{"item":album,"name":"Album"}],
                "artists":[{"item":artist,"name":"Artist"}],
                "songs":songs,"song_artists":relations,
                "album_artists":[{"owner":album,"mode":"replace","targets":[artist]}],
                "images":[{"item_id":album,"image_type":"Primary","native_id":"picture"}],
            }))
            .unwrap(),
        )
        .await
        .unwrap();
    m::item::Entity::update_many()
        .col_expr(m::item::Column::LastSyncAt, sea_query::Expr::value(0))
        .exec(&ctx.db)
        .await
        .unwrap();
    m::library::Entity::update_many()
        .col_expr(
            m::library::Column::EditTime,
            sea_query::Expr::value(chrono::Utc::now() + chrono::Duration::days(1)),
        )
        .exec(&ctx.db)
        .await
        .unwrap();
    let ids = result.songs.iter().map(|song| song.id).collect();
    let owner = Arc::new(RadioSessions {
        validate_provider: Arc::new(|_| true),
        ..Default::default()
    });
    let provider = Arc::new(TestProvider::default());
    provider.set_id(Some(PROVIDER));
    let target = Target {
        queue_id: 100,
        native_id: "roaming/fm".into(),
        provider: provider.clone(),
    };
    (owner, ctx, target, provider, library, ids)
}

async fn prune(ctx: &Context, library: i64, explicit: bool) {
    if explicit {
        let mut run = ctx.sync_coordinator.start(PROVIDER).await.unwrap();
        run.observe_libraries(&ctx.db, vec![library]).await.unwrap();
        let kinds = [ItemType::Song, ItemType::Album, ItemType::Artist];
        let mut scopes: Vec<_> = kinds
            .iter()
            .map(|kind| SyncScope::new(Some(library), *kind))
            .collect();
        scopes.push(SyncScope::new(None, ItemType::Mix));
        run.declare(scopes.clone(), &kinds).unwrap();
        for scope in scopes {
            run.complete(scope).unwrap();
        }
        run.complete_libraries().unwrap();
        run.seal().unwrap();
        run.finalize(&ctx.db, &ctx.sync_coordinator).await.unwrap();
    } else {
        let write = ctx.sync_coordinator.write(PROVIDER, None).await.unwrap();
        let txn = ctx.db.begin().await.unwrap();
        qcm_core::db::sync::sync_drop_before(
            &txn,
            PROVIDER,
            chrono::Utc::now(),
            &write.retained_items(),
        )
        .await
        .unwrap();
        txn.commit().await.unwrap();
    }
}

async fn playable(ctx: &Context, id: i64) {
    let song = m::song::Entity::find_by_id(id)
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    let item = m::item::Entity::find_by_id(id)
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    assert!(!item.native_id.is_empty());
    let album = song.album_id.unwrap();
    assert!(
        m::album::Entity::find_by_id(album)
            .one(&ctx.db)
            .await
            .unwrap()
            .is_some()
    );
    assert_eq!(
        m::image::Entity::find()
            .filter(m::image::Column::ItemId.eq(album))
            .one(&ctx.db)
            .await
            .unwrap()
            .unwrap()
            .native_id
            .as_deref(),
        Some("picture")
    );
    assert_eq!(
        m::rel_song_artist::Entity::find()
            .filter(m::rel_song_artist::Column::SongId.eq(id))
            .count(&ctx.db)
            .await
            .unwrap(),
        1
    );
    assert_eq!(
        m::rel_album_artist::Entity::find()
            .filter(m::rel_album_artist::Column::AlbumId.eq(album))
            .count(&ctx.db)
            .await
            .unwrap(),
        1
    );
}

#[tokio::test]
async fn prepared_radios_survive_full_sync_and_release_only_unused_items() {
    for explicit in [false, true] {
        let (owner, ctx, target, provider, library, ids) = fixture().await;
        *provider.songs.lock().unwrap() = vec![ids[0]];
        let private = open(&owner, &ctx, &target).await;
        let mut similar_target = target.clone();
        similar_target.queue_id += 1;
        similar_target.native_id = "roaming/similar".into();
        *provider.songs.lock().unwrap() = vec![ids[1]];
        *provider.source.lock().unwrap() = Some(ids[2]);
        let similar = open(&owner, &ctx, &similar_target).await;
        prune(&ctx, library, explicit).await;
        for id in &ids[..3] {
            playable(&ctx, *id).await;
        }
        for id in &ids[3..] {
            assert!(
                m::item::Entity::find_by_id(*id)
                    .one(&ctx.db)
                    .await
                    .unwrap()
                    .is_none()
            );
        }
        provider.fail.store(true, Ordering::Release);
        assert!(
            owner
                .next(
                    ctx.clone(),
                    target.clone(),
                    private.session_id.clone(),
                    operation_id(),
                    1,
                    vec!["private".into()]
                )
                .await
                .is_err()
        );
        prune(&ctx, library, explicit).await;
        playable(&ctx, ids[0]).await;
        owner.close(&ctx, &private.session_id).await;
        prune(&ctx, library, explicit).await;
        assert!(
            m::song::Entity::find_by_id(ids[0])
                .one(&ctx.db)
                .await
                .unwrap()
                .is_none()
        );
        playable(&ctx, ids[1]).await;
        playable(&ctx, ids[2]).await;
        owner.close(&ctx, &similar.session_id).await;
        prune(&ctx, library, explicit).await;
        assert_eq!(m::song::Entity::find().count(&ctx.db).await.unwrap(), 0);
        assert_eq!(m::album::Entity::find().count(&ctx.db).await.unwrap(), 0);
        assert_eq!(m::artist::Entity::find().count(&ctx.db).await.unwrap(), 0);
        assert_eq!(m::image::Entity::find().count(&ctx.db).await.unwrap(), 0);
        assert_eq!(
            m::rel_song_artist::Entity::find()
                .count(&ctx.db)
                .await
                .unwrap(),
            0
        );
    }
}

#[tokio::test]
async fn continuation_releases_departed_songs_and_retention_does_not_conflict_with_sync() {
    let (owner, ctx, target, provider, library, ids) = fixture().await;
    *provider.songs.lock().unwrap() = vec![ids[0]];
    let first = open(&owner, &ctx, &target).await;
    *provider.songs.lock().unwrap() = vec![ids[3]];
    let second = owner
        .next(
            ctx.clone(),
            target.clone(),
            first.session_id.clone(),
            operation_id(),
            1,
            vec!["private".into()],
        )
        .await
        .unwrap();
    prune(&ctx, library, false).await;
    playable(&ctx, ids[0]).await;
    playable(&ctx, ids[3]).await;
    owner
        .next(
            ctx.clone(),
            target,
            first.session_id.clone(),
            operation_id(),
            second.version,
            vec!["next".into()],
        )
        .await
        .unwrap();
    prune(&ctx, library, true).await;
    assert!(
        m::song::Entity::find_by_id(ids[0])
            .one(&ctx.db)
            .await
            .unwrap()
            .is_none()
    );
    playable(&ctx, ids[3]).await;
    let mut run = ctx.sync_coordinator.start(PROVIDER).await.unwrap();
    let revision = run.revision();
    let retained = ctx
        .sync_coordinator
        .retain_items(&ctx.db, PROVIDER, [ids[3]])
        .await
        .unwrap();
    assert_eq!(run.revision(), revision);
    run.observe_libraries(&ctx.db, vec![library]).await.unwrap();
    let scope = SyncScope::new(None, ItemType::Mix);
    run.declare(vec![scope.clone()], &[]).unwrap();
    run.complete(scope).unwrap();
    run.complete_libraries().unwrap();
    run.seal().unwrap();
    run.finalize(&ctx.db, &ctx.sync_coordinator).await.unwrap();
    assert!(
        ctx.sync_coordinator
            .retain_items(&ctx.db, 2, [ids[3]])
            .await
            .is_err()
    );
    assert!(
        ctx.sync_coordinator
            .retain_items(&ctx.db, PROVIDER, [i64::MAX])
            .await
            .is_err()
    );
    owner.close(&ctx, &first.session_id).await;
    prune(&ctx, library, false).await;
    playable(&ctx, ids[3]).await;
    drop(retained);
    prune(&ctx, library, false).await;
    assert!(
        m::song::Entity::find_by_id(ids[3])
            .one(&ctx.db)
            .await
            .unwrap()
            .is_none()
    );
}

#[tokio::test]
async fn audio_and_song_cover_http_remain_available_after_sync() {
    use crate::{event::ServiceContext, http::body_type::ResponseBody, task::TaskManager};
    use http_body_util::{BodyExt, Full};
    use hyper::{Response, body::Bytes};
    use hyper_util::rt::TokioIo;

    let (owner, ctx, target, provider, library, ids) = fixture().await;
    *provider.songs.lock().unwrap() = vec![ids[0]];
    let private = open(&owner, &ctx, &target).await;
    *provider.songs.lock().unwrap() = vec![ids[1]];
    *provider.source.lock().unwrap() = Some(ids[2]);
    let similar = open(
        &owner,
        &ctx,
        &Target {
            queue_id: 101,
            native_id: "roaming/similar".into(),
            provider: provider.clone(),
        },
    )
    .await;
    qcm_core::global::add_provider(provider.clone());
    let _registered = scopeguard::guard((), |_| qcm_core::global::remove_provider(PROVIDER));
    let directory = std::env::temp_dir().join(format!("qcm-radio-media-{}", uuid::Uuid::new_v4()));
    let cache = crate::resource_cache::ResourceCache::open(directory.clone())
        .await
        .unwrap();
    let (oper, _) = TaskManager::new();
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let service = Arc::new(ServiceContext {
        provider_context: ctx.clone(),
        backend_ev,
        oper,
        mix_refresh: Default::default(),
        resource_cache: cache.clone(),
        playback: Arc::new(Default::default()),
        radio: owner.clone(),
    });
    crate::global::reg_context(1, service.clone());
    let _context = scopeguard::guard((), |_| crate::global::unreg_context(1));
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let base = format!("http://{}", listener.local_addr().unwrap());
    *provider.media.lock().unwrap() = Some(format!("{base}/origin"));
    let server = tokio::spawn(async move {
        let mut connections = tokio::task::JoinSet::new();
        while let Ok((socket, _)) = listener.accept().await {
            let ctx = service.clone();
            connections.spawn(async move {
                let service = hyper::service::service_fn(
                    move |request: hyper::Request<hyper::body::Incoming>| {
                        let ctx = ctx.clone();
                        async move {
                            if request.uri().path().starts_with("/origin/") {
                                let image = request.uri().path().contains("/image/");
                                let payload: &[u8] = if image {
                                    b"image payload"
                                } else {
                                    b"audio payload"
                                };
                                Ok(Response::builder()
                                    .header(
                                        "content-type",
                                        if image { "image/png" } else { "audio/wav" },
                                    )
                                    .body(ResponseBody::Boxed(
                                        Full::new(Bytes::copy_from_slice(payload))
                                            .map_err(|e| e.into())
                                            .boxed(),
                                    ))
                                    .unwrap())
                            } else {
                                crate::api::handler::handle_request(
                                    request,
                                    ctx.provider_context.db.clone(),
                                    ctx.provider_context.cache_db.clone(),
                                    ctx.oper.clone(),
                                    ctx.resource_cache.clone(),
                                )
                                .await
                            }
                        }
                    },
                );
                let _ = hyper::server::conn::http1::Builder::new()
                    .serve_connection(TokioIo::new(socket), service)
                    .await;
            });
        }
    });
    let client = reqwest::Client::new();
    for after_sync in [false, true] {
        if after_sync {
            prune(&ctx, library, false).await;
        }
        for id in [ids[0], ids[1]] {
            for (path, payload) in [
                (format!("/audio/Song/{id}"), "audio payload"),
                (format!("/image/Song/{id}/Primary"), "image payload"),
            ] {
                let response = client.get(format!("{base}{path}")).send().await.unwrap();
                assert_eq!(response.status(), 200, "{path}, after_sync={after_sync}");
                assert_eq!(response.text().await.unwrap(), payload);
            }
        }
    }
    owner.close(&ctx, &private.session_id).await;
    owner.close(&ctx, &similar.session_id).await;
    server.abort();
    let _ = server.await;
    cache.shutdown().await.unwrap();
    std::fs::remove_dir_all(directory).unwrap();
}

#[tokio::test]
async fn retention_does_not_keep_obsolete_relations_for_items_enumerated_by_sync() {
    let (owner, ctx, target, provider, library, ids) = fixture().await;
    *provider.songs.lock().unwrap() = vec![ids[0]];
    let page = open(&owner, &ctx, &target).await;
    let song = m::song::Entity::find_by_id(ids[0])
        .one(&ctx.db)
        .await
        .unwrap()
        .unwrap();
    let album = song.album_id.unwrap();
    let observed: qcm_core::db::values::Timestamp =
        (chrono::Utc::now() + chrono::Duration::days(1)).into();
    m::item::Entity::update_many()
        .col_expr(
            m::item::Column::LastSyncAt,
            sea_query::Expr::value(observed),
        )
        .filter(m::item::Column::Id.is_in([ids[0], album]))
        .exec(&ctx.db)
        .await
        .unwrap();
    m::rel_song_artist::Entity::update_many()
        .col_expr(
            m::rel_song_artist::Column::UpdateAt,
            sea_query::Expr::value(0),
        )
        .exec(&ctx.db)
        .await
        .unwrap();
    m::rel_album_artist::Entity::update_many()
        .col_expr(
            m::rel_album_artist::Column::UpdateAt,
            sea_query::Expr::value(0),
        )
        .exec(&ctx.db)
        .await
        .unwrap();
    prune(&ctx, library, false).await;
    assert!(
        m::song::Entity::find_by_id(ids[0])
            .one(&ctx.db)
            .await
            .unwrap()
            .is_some()
    );
    assert_eq!(
        m::rel_song_artist::Entity::find()
            .filter(m::rel_song_artist::Column::SongId.eq(ids[0]))
            .count(&ctx.db)
            .await
            .unwrap(),
        0
    );
    assert_eq!(
        m::rel_album_artist::Entity::find()
            .filter(m::rel_album_artist::Column::AlbumId.eq(album))
            .count(&ctx.db)
            .await
            .unwrap(),
        0
    );
    owner.close(&ctx, &page.session_id).await;
}
