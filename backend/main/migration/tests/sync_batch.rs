use migration::sea_orm::{self, *};
use migration::{Migrator, MigratorTrait};
use qcm_core::{
    db::{
        fts,
        sync_batch::{ProviderWriter, SyncBatch},
        sync_run::SyncCoordinator,
    },
    model as m,
};
use serde_json::{json, Value};

async fn database() -> DatabaseConnection {
    let mut opts = ConnectOptions::new("sqlite::memory:");
    opts.max_connections(1);
    let db = Database::connect(opts).await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=ON")
        .await
        .unwrap();
    Migrator::up(&db, Some(4)).await.unwrap();
    migration::MigrationTrait::up(
        &migration::m20261007_000001_image_type_integer::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
    migration::MigrationTrait::up(&migration::m20261017_000001_provider_session::Migration, &migration::SchemaManager::new(&db)).await.unwrap();
    // Exercise production FTS triggers; tokenization itself is tested by the backend.
    for (table, columns) in [
        ("album", vec!["name", "description"]),
        ("song", vec!["name"]),
        ("artist", vec!["name", "description"]),
        ("mix", vec!["name", "description"]),
    ] {
        let sql = fts::create_fts_table_sql(table, &columns)
            .replace("tokenize='qcm'", "tokenize='unicode61'");
        db.execute_unprepared(&sql).await.unwrap();
        fts::create_fts_triggers(&db, table, &columns)
            .await
            .unwrap();
    }
    for id in [1, 2] {
        m::provider::ActiveModel {
            provider_id: Set(id),
            name: Set(format!("provider{id}")),
            type_: Set("test".into()),
            base_url: Set(String::new()),
            auth_method: Set(None),
            cookie: Set(String::new()),
        session_key: Set(String::new()),
            custom: Set(String::new()),
            edit_time: Set(chrono_now()),
        }
        .insert(&db)
        .await
        .unwrap();
    }
    db
}
fn chrono_now() -> sea_orm::prelude::DateTimeUtc {
    std::time::SystemTime::now().into()
}
fn batch(value: Value) -> SyncBatch {
    serde_json::from_value(value).unwrap()
}
fn key(library: i64, kind: &str, native: &str) -> Value {
    json!({"library_id":library,"type":kind,"native_id":native})
}

#[tokio::test]
async fn explicit_radio_scope_keeps_identity_and_requires_completion() {
    use qcm_core::{db::sync_run::SyncScope, model::type_enum::ItemType};
    let db = database().await;
    let coordinator = SyncCoordinator::default();
    let lib = library(&db, &coordinator, 1, "music").await;
    let radio = json!({"library_id":null,"type":"Radio","native_id":"random"});
    let result = ProviderWriter::new(&db, 1, &coordinator).sync_batch(batch(json!({
        "remote_mixes":[{"item":radio,"name":"Random","mix_type":"queue"}]
    }))).await.unwrap();
    let id = result.items[0].id;
    let song_scope = SyncScope::new(Some(lib), ItemType::Song);
    let mix_scope = SyncScope::new(None, ItemType::Mix);
    let radio_scope = SyncScope::new(None, ItemType::Radio);
    for invalid in [ItemType::Song, ItemType::Artist, ItemType::Album] {
        let mut run = coordinator.start(1).await.unwrap();
        run.observe_libraries(&db, vec![lib]).await.unwrap();
        assert!(run.declare(vec![song_scope.clone(), mix_scope.clone(), SyncScope::new(None, invalid)], &[ItemType::Song]).is_err());
    }
    let mut run = coordinator.start(1).await.unwrap();
    run.observe_libraries(&db, vec![lib]).await.unwrap();
    run.declare(vec![song_scope.clone(), mix_scope.clone(), radio_scope.clone()], &[ItemType::Song]).unwrap();
    run.complete_libraries().unwrap();
    run.observe_items(&db, &coordinator, &radio_scope, &[serde_json::from_value(radio).unwrap()]).await.unwrap();
    run.complete(song_scope.clone()).unwrap();
    run.complete(mix_scope.clone()).unwrap();
    assert!(run.seal().is_err());
    run.complete(radio_scope.clone()).unwrap();
    run.seal().unwrap();
    run.finalize(&db, &coordinator).await.unwrap();
    assert!(m::item::Entity::find_by_id(id).one(&db).await.unwrap().is_some());
    drop(run);
    let mut run = coordinator.start(1).await.unwrap();
    run.observe_libraries(&db, vec![lib]).await.unwrap();
    run.declare(vec![song_scope.clone(), mix_scope.clone(), radio_scope.clone()], &[ItemType::Song]).unwrap();
    run.complete_libraries().unwrap();
    for scope in [song_scope,mix_scope,radio_scope] { run.complete(scope).unwrap(); }
    run.seal().unwrap();
    run.finalize(&db, &coordinator).await.unwrap();
    assert!(m::item::Entity::find_by_id(id).one(&db).await.unwrap().is_none());
}
async fn library(
    db: &DatabaseConnection,
    coordinator: &SyncCoordinator,
    provider: i64,
    native: &str,
) -> i64 {
    ProviderWriter::new(db, provider, coordinator)
        .sync_batch(batch(
            json!({"libraries":[{"native_id":native,"name":"Library"}]}),
        ))
        .await
        .unwrap()
        .libraries[0]
        .library_id
}
#[tokio::test]
async fn mix_local_edits_update_only_the_changed_mix() {
    use qcm_core::{db::values::Timestamp, model::type_enum::MixType};

    let db = database().await;
    let coordinator = SyncCoordinator::default();
    let lib = library(&db, &coordinator, 1, "local").await;
    let result = ProviderWriter::new(&db, 1, &coordinator)
        .sync_batch(batch(
            json!({"songs":[{"item":key(lib,"Song","song"),"name":"Song"}]}),
        ))
        .await
        .unwrap();
    let song = result.songs[0].id;
    let old = Timestamp::from_millis(1_000);
    let mut mixes = Vec::new();
    for name in ["Changed", "Untouched"] {
        mixes.push(
            m::mix::ActiveModel {
                name: Set(name.into()),
                mix_type: Set(MixType::Normal),
                track_count: Set(0),
                description: Set(String::new()),
                update_at: Set(old),
                ..Default::default()
            }
            .insert(&db)
            .await
            .unwrap(),
        );
    }
    let id = mixes[0].id;
    let txn = db.begin().await.unwrap();
    assert_eq!(
        m::mix::append_songs(&txn, id, &vec![song]).await.unwrap(),
        1
    );
    txn.commit().await.unwrap();
    let changed = m::mix::Entity::find_by_id(id)
        .one(&db)
        .await
        .unwrap()
        .unwrap();
    assert_eq!(changed.track_count, 1);
    assert!(changed.update_at.as_millis() > old.as_millis());
    assert_eq!(changed.content_update_at, mixes[0].content_update_at);

    let txn = db.begin().await.unwrap();
    assert_eq!(
        m::mix::append_songs(&txn, id, &vec![song]).await.unwrap(),
        0
    );
    txn.commit().await.unwrap();
    assert_eq!(
        m::mix::Entity::find_by_id(id)
            .one(&db)
            .await
            .unwrap()
            .unwrap(),
        changed
    );

    m::mix::ActiveModel {
        id: Set(id),
        update_at: Set(old),
        ..Default::default()
    }
    .update(&db)
    .await
    .unwrap();
    let txn = db.begin().await.unwrap();
    assert_eq!(m::mix::remove_songs(&txn, id, &[song]).await.unwrap(), 1);
    txn.commit().await.unwrap();
    let removed = m::mix::Entity::find_by_id(id)
        .one(&db)
        .await
        .unwrap()
        .unwrap();
    assert_eq!(removed.track_count, 0);
    assert!(removed.update_at.as_millis() > old.as_millis());
    let txn = db.begin().await.unwrap();
    assert_eq!(m::mix::remove_songs(&txn, id, &[song]).await.unwrap(), 0);
    txn.commit().await.unwrap();
    assert_eq!(
        m::mix::Entity::find_by_id(id)
            .one(&db)
            .await
            .unwrap()
            .unwrap(),
        removed
    );
    assert_eq!(
        m::mix::Entity::find_by_id(mixes[1].id)
            .one(&db)
            .await
            .unwrap()
            .unwrap(),
        mixes[1]
    );
}

#[tokio::test]
async fn mix_local_update_time_advances_on_content_sync() {
    use qcm_core::db::values::Timestamp;

    let db = database().await;
    let coordinator = SyncCoordinator::default();
    let writer = ProviderWriter::new(&db, 1, &coordinator);
    let remote = json!({"type":"Mix","native_id":"mix","library_id":null});
    let result = writer
        .sync_batch(batch(
            json!({"remote_mixes":[{"item":remote,"name":"Mix"}]}),
        ))
        .await
        .unwrap();
    let id = result.remote_mixes[0].id;
    let mix = m::mix::Entity::find()
        .filter(m::mix::Column::RemoteId.eq(id))
        .one(&db)
        .await
        .unwrap()
        .unwrap();
    let old = Timestamp::from_millis(1_000);
    m::mix::ActiveModel {
        id: Set(mix.id),
        update_at: Set(old),
        content_update_at: Set(old),
        ..Default::default()
    }
    .update(&db)
    .await
    .unwrap();
    writer
        .sync_batch(batch(
            json!({"remote_mix_contents":[{"mix":id,"mode":"replace","songs":[]}]}),
        ))
        .await
        .unwrap();
    let updated = m::mix::Entity::find_by_id(mix.id)
        .one(&db)
        .await
        .unwrap()
        .unwrap();
    assert!(updated.update_at.as_millis() > old.as_millis());
    assert_eq!(updated.update_at, updated.content_update_at);
}

#[tokio::test]
async fn identity_patches_masks_and_local_state() {
    let db = database().await;
    let coordinator = SyncCoordinator::default();
    let lib = library(&db, &coordinator, 1, "one").await;
    let writer = ProviderWriter::new(&db, 1, &coordinator);
    let album = key(lib, "Album", "same");
    let song = key(lib, "Song", "same");
    let out = writer
        .sync_batch(batch(json!({
            "albums":[{"item":album,"name":"Album","description":"full"}],
            "songs":[{"item":song,"name":"Song","duration":123000000,"album_id":album}],
            "dynamics":[{"id":song,"on_insert":{"is_external":true},"favorite_at":0}]
        })))
        .await
        .unwrap();
    let song_id = out.songs[0].id;
    assert_eq!(out.songs[0].album_id, Some(out.albums[0].id));
    assert_ne!(song_id, out.albums[0].id);
    let created = out
        .items
        .iter()
        .find(|i| i.id == song_id)
        .unwrap()
        .create_at;
    m::dynamic::ActiveModel {
        id: Set(song_id),
        play_count: Set(7),
        is_external: Set(false),
        ..Default::default()
    }
    .update(&db)
    .await
    .unwrap();
    let repeated = writer
        .sync_batch(batch(json!({
            "albums":[{"item":album,"name":"Renamed","description":null}],
            "songs":[{"item":song,"name":"Song 2"}],
            "dynamics":[{"id":song,"on_insert":{"is_external":true}}],
            "options":{"albums":{"include":["name"]}}
        })))
        .await
        .unwrap();
    assert_eq!(repeated.songs[0].id, song_id);
    assert_eq!(repeated.songs[0].duration, 123000000);
    assert_eq!(repeated.albums[0].description.as_deref(), Some("full"));
    assert_eq!(
        repeated
            .items
            .iter()
            .find(|i| i.id == song_id)
            .unwrap()
            .create_at,
        created
    );
    let dynamic = m::dynamic::Entity::find_by_id(song_id)
        .one(&db)
        .await
        .unwrap()
        .unwrap();
    assert_eq!(dynamic.play_count, 7);
    assert!(!dynamic.is_external);
    assert!(dynamic.favorite_at.is_some());
    writer
        .sync_batch(batch(
            json!({"dynamics":[{"id":song_id,"favorite_at":null}]}),
        ))
        .await
        .unwrap();
    assert!(m::dynamic::Entity::find_by_id(song_id)
        .one(&db)
        .await
        .unwrap()
        .unwrap()
        .favorite_at
        .is_none());
    assert!(serde_json::from_value::<SyncBatch>(
        json!({"dynamics":[{"id":song_id,"play_count":0}]})
    )
    .is_err());
    let other_lib = library(&db, &coordinator, 2, "one").await;
    assert!(writer
        .sync_batch(batch(
            json!({"songs":[{"item":key(other_lib,"Song","same"),"name":"bad"}]})
        ))
        .await
        .is_err());
    let other = ProviderWriter::new(&db, 2, &coordinator)
        .sync_batch(batch(
            json!({"songs":[{"item":key(other_lib,"Song","same"),"name":"Other"}]}),
        ))
        .await
        .unwrap();
    assert_ne!(song_id, other.songs[0].id);
    assert!(writer
        .sync_batch(batch(
            json!({"songs":[{"item":other.songs[0].id,"name":"bad"}]})
        ))
        .await
        .is_err());
}
#[tokio::test]
async fn relations_images_roll_back_and_mix_replace_is_complete() {
    let db = database().await;
    let coordinator = SyncCoordinator::default();
    let lib = library(&db, &coordinator, 1, "one").await;
    let writer = ProviderWriter::new(&db, 1, &coordinator);
    let song = key(lib, "Song", "song");
    let out = writer
        .sync_batch(batch(json!({"songs":[{"item":song,"name":"Song"}]})))
        .await
        .unwrap();
    let id = out.songs[0].id;
    let failed = writer.sync_batch(batch(json!({"songs":[{"item":key(lib,"Song","rolled_back"),"name":"Bad"}],"images":[{"item_id":999999,"image_type":"Primary","native_id":"bad"}]}))).await;
    assert!(failed.is_err());
    assert_eq!(m::item::Entity::find().count(&db).await.unwrap(), 1);
    let mix = json!({"type":"Mix","native_id":"mix","library_id":null});
    let result = writer.sync_batch(batch(json!({"remote_mixes":[{"item":mix,"name":"Mix"}],"remote_mix_contents":[{"mix":mix,"mode":"replace","songs":[song]}]}))).await.unwrap();
    let remote = result.remote_mixes[0].id;
    assert_eq!(result.remote_mixes[0].track_count, 1);
    assert_eq!(m::rel_mix_song::Entity::find().count(&db).await.unwrap(), 1);
    assert!(writer
        .sync_batch(batch(
            json!({"remote_mix_contents":[{"mix":remote,"mode":"replace","songs":[id,id]}]})
        ))
        .await
        .is_err());
    assert_eq!(m::rel_mix_song::Entity::find().count(&db).await.unwrap(), 1);
    writer
        .sync_batch(batch(
            json!({"remote_mix_contents":[{"mix":remote,"mode":"replace","songs":[]}]}),
        ))
        .await
        .unwrap();
    assert_eq!(m::rel_mix_song::Entity::find().count(&db).await.unwrap(), 0);
    assert_eq!(
        m::remote_mix::Entity::find_by_id(remote)
            .one(&db)
            .await
            .unwrap()
            .unwrap()
            .track_count,
        0
    );
    let rows = db
        .query_all(Statement::from_string(
            DatabaseBackend::Sqlite,
            "SELECT rowid FROM song_fts WHERE song_fts MATCH 'Song'".to_owned(),
        ))
        .await
        .unwrap();
    assert_eq!(rows.len(), 1);
}

#[tokio::test]
async fn explicit_sync_requires_complete_scopes_and_rejects_conflicting_prune() {
    use qcm_core::{db::sync_run::SyncScope, model::type_enum::ItemType};
    let db = database().await;
    let coordinator = SyncCoordinator::default();
    let lib = library(&db, &coordinator, 1, "one").await;
    let writer = ProviderWriter::new(&db, 1, &coordinator);
    let out = writer
        .sync_batch(batch(
            json!({"songs":[{"item":key(lib,"Song","old"),"name":"Old"}]}),
        ))
        .await
        .unwrap();
    let id = out.songs[0].id;
    let song_scope = SyncScope::new(Some(lib), ItemType::Song);
    let mix_scope = SyncScope::new(None, ItemType::Mix);
    let mut run = coordinator.start(1).await.unwrap();
    assert!(coordinator.start(1).await.is_err());
    run.observe_libraries(&db, vec![lib]).await.unwrap();
    run.declare(
        vec![song_scope.clone(), mix_scope.clone()],
        &[ItemType::Song],
    )
    .unwrap();
    assert!(run.seal().is_err());
    run.complete_libraries().unwrap();
    run.complete(song_scope.clone()).unwrap();
    run.complete(mix_scope.clone()).unwrap();
    run.seal().unwrap();
    writer
        .sync_batch(batch(json!({"songs":[{"item":id,"name":"Changed"}]})))
        .await
        .unwrap();
    assert!(run.finalize(&db, &coordinator).await.is_err());
    assert!(m::song::Entity::find_by_id(id)
        .one(&db)
        .await
        .unwrap()
        .is_some());
    drop(run);
    let mut run = coordinator.start(1).await.unwrap();
    run.observe_libraries(&db, vec![lib]).await.unwrap();
    run.declare(
        vec![song_scope.clone(), mix_scope.clone()],
        &[ItemType::Song],
    )
    .unwrap();
    run.complete_libraries().unwrap();
    run.complete(song_scope).unwrap();
    run.complete(mix_scope).unwrap();
    run.seal().unwrap();
    run.finalize(&db, &coordinator).await.unwrap();
    assert!(m::song::Entity::find_by_id(id)
        .one(&db)
        .await
        .unwrap()
        .is_none());
}
#[tokio::test]
async fn ui_favorite_wins_over_older_observation() {
    let db = database().await;
    let coordinator = SyncCoordinator::default();
    let lib = library(&db, &coordinator, 1, "one").await;
    let result = ProviderWriter::new(&db, 1, &coordinator)
        .sync_batch(batch(
            json!({"songs":[{"item":key(lib,"Song","song"),"name":"Song"}]}),
        ))
        .await
        .unwrap();
    let id = result.songs[0].id;
    let mut run = coordinator.start(1).await.unwrap();
    {
        let mut guard = coordinator.write(1, None).await.unwrap();
        guard.favorite(id);
        m::dynamic::ActiveModel {
            id: Set(id),
            favorite_at: Set(Some(qcm_core::db::values::Timestamp::from_millis(123))),
            ..Default::default()
        }
        .update(&db)
        .await
        .unwrap();
    }
    run.sync_batch(
        &db,
        &coordinator,
        batch(json!({"dynamics":[{"id":id,"favorite_at":null}]})),
    )
    .await
    .unwrap();
    assert_eq!(
        m::dynamic::Entity::find_by_id(id)
            .one(&db)
            .await
            .unwrap()
            .unwrap()
            .favorite_at
            .unwrap()
            .as_millis(),
        123
    );
}

#[tokio::test]
async fn canceled_and_replaced_runs_never_prune() {
    use qcm_core::db::sync_run::{SyncScope, WriteGeneration};
    use qcm_core::model::type_enum::ItemType;
    use std::sync::{
        atomic::{AtomicU64, Ordering},
        Arc,
    };
    let db = database().await;
    let c = SyncCoordinator::default();
    let lib = library(&db, &c, 1, "one").await;
    let id = ProviderWriter::new(&db, 1, &c)
        .sync_batch(batch(
            json!({"songs":[{"item":key(lib,"Song","keep"),"name":"Keep"}]}),
        ))
        .await
        .unwrap()
        .songs[0]
        .id;
    for cancel in [true, false] {
        let epoch = Arc::new(AtomicU64::new(1));
        let mut run = c.start(1).await.unwrap();
        run.bind_generation(WriteGeneration {
            current: epoch.clone(),
            expected: 1,
        });
        run.observe_libraries(&db, vec![lib]).await.unwrap();
        let scopes = vec![
            SyncScope::new(Some(lib), ItemType::Song),
            SyncScope::new(None, ItemType::Mix),
        ];
        run.declare(scopes.clone(), &[ItemType::Song]).unwrap();
        run.complete_libraries().unwrap();
        for scope in scopes {
            run.complete(scope).unwrap();
        }
        run.seal().unwrap();
        if cancel {
            run.cancellation().cancel();
        } else {
            epoch.fetch_add(1, Ordering::AcqRel);
        }
        assert!(run.finalize(&db, &c).await.is_err());
        assert!(m::song::Entity::find_by_id(id)
            .one(&db)
            .await
            .unwrap()
            .is_some());
    }
}

#[tokio::test]
async fn invalid_batches_are_atomic() {
    let db = database().await;
    let c = SyncCoordinator::default();
    let lib = library(&db, &c, 1, "one").await;
    let w = ProviderWriter::new(&db, 1, &c);
    for fields in [
        json!({"duration":-1}),
        json!({"tags":null}),
        json!({"tags":[1]}),
    ] {
        let mut song = json!({"item":key(lib,"Song","bad"),"name":"Bad"});
        song.as_object_mut()
            .unwrap()
            .extend(fields.as_object().unwrap().clone());
        match serde_json::from_value::<SyncBatch>(json!({"songs":[song]})) {
            Ok(input) => assert!(w.sync_batch(input).await.is_err()),
            Err(_) => {}
        }
        assert_eq!(m::item::Entity::find().count(&db).await.unwrap(), 0);
    }
    assert!(w
        .sync_batch(batch(json!({"options":{"songs":{"include":["unknown"]}}})))
        .await
        .is_err());
}

#[tokio::test]
async fn image_migration_preserves_identity_and_foreign_keys() {
    let db = Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=ON")
        .await
        .unwrap();
    Migrator::up(&db, Some(4)).await.unwrap();
    db.execute_unprepared("INSERT INTO provider(provider_id,name,type,base_url,auth_method,cookie,custom,edit_time) VALUES (1,'test','test','',NULL,'','',CURRENT_TIMESTAMP);
      INSERT INTO library(library_id,provider_id,name,native_id,edit_time) VALUES(1,1,'music','music',CURRENT_TIMESTAMP);
      INSERT INTO item(id,native_id,provider_id,library_id,type,create_at,update_at,last_sync_at) VALUES(1,'song',1,1,2,0,0,0);
      INSERT INTO image(id,item_id,image_type,native_id,db,fresh,timestamp) VALUES(19,1,'0','cover',NULL,'tag',CURRENT_TIMESTAMP);").await.unwrap();
    migration::MigrationTrait::up(
        &migration::m20261007_000001_image_type_integer::Migration,
        &migration::SchemaManager::new(&db),
    )
    .await
    .unwrap();
    let image = m::image::Entity::find_by_id(19)
        .one(&db)
        .await
        .unwrap()
        .unwrap();
    assert_eq!(image.image_type, m::type_enum::ImageType::Primary);
    assert_eq!(image.native_id.as_deref(), Some("cover"));
    m::item::Entity::delete_by_id(1).exec(&db).await.unwrap();
    assert_eq!(m::image::Entity::find().count(&db).await.unwrap(), 0);
}
