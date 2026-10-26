use super::process_qcm::process_qcm;
use crate::{event::ServiceContext, msg};
use prost::Message;
use qcm_core::{model as sqlm, provider::Context};
use sea_orm::{ConnectOptions, ConnectionTrait, Database, Schema};
use std::sync::Arc;

#[tokio::test]
async fn filtered_song_ids_follow_requested_order() {
    let mut options = ConnectOptions::new("sqlite::memory:");
    options.max_connections(1);
    let db = Database::connect(options).await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=OFF")
        .await
        .unwrap();
    let schema = Schema::new(db.get_database_backend());
    for table in [
        schema.create_table_from_entity(sqlm::song::Entity),
        schema.create_table_from_entity(sqlm::item::Entity),
        schema.create_table_from_entity(sqlm::album::Entity),
        schema.create_table_from_entity(sqlm::artist::Entity),
        schema.create_table_from_entity(sqlm::dynamic::Entity),
        schema.create_table_from_entity(sqlm::mix::Entity),
        schema.create_table_from_entity(sqlm::rel_album_artist::Entity),
        schema.create_table_from_entity(sqlm::rel_song_artist::Entity),
    ] {
        db.execute(db.get_database_backend().build(&table))
            .await
            .unwrap();
    }
    db.execute_unprepared(
        "INSERT INTO song(id,name,album_id,disc_number,track_number,duration,can_play,popularity,tags) VALUES
         (10,'C',1,2,1,1000,1,0,'[]'),
         (20,'A',1,1,2,1000,1,0,'[]'),
         (30,'B',1,1,1,1000,1,0,'[]'),
         (40,'D',1,2,1,1000,1,0,'[]'),
         (50,'Other album',2,1,1,1000,1,0,'[]');
         INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at)
         SELECT id,CAST(id AS TEXT),1,101,0,0,0 FROM song;
         INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at) VALUES(1,'album',1,51,0,0,0);
         INSERT INTO album(id,name,track_count,disc_count,type,duration)
         VALUES(1,'Album',4,2,0,4000);",
    )
    .await
    .unwrap();

    let directory = std::env::temp_dir().join(format!("qcm-song-order-{}", uuid::Uuid::new_v4()));
    let _cleanup = scopeguard::guard(directory.clone(), |path| {
        let _ = std::fs::remove_dir_all(path);
    });
    let cache = crate::resource_cache::ResourceCache::open(directory)
        .await
        .unwrap();
    let (events, _) = tokio::sync::mpsc::channel(1);
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let (oper, _) = crate::task::TaskManager::new();
    let service = Arc::new(ServiceContext {
        provider_context: Arc::new(Context {
            db: db.clone(),
            cache_db: db,
            ev_sender: events,
            sync_coordinator: Default::default(),
        }),
        backend_ev,
        oper,
        mix_refresh: Default::default(),
        resource_cache: cache,
        radio: Default::default(),
        playback: Default::default(),
    });

    for (sort, asc, expected) in [
        (
            msg::model::SongSort::TrackNumber,
            true,
            vec![30, 20, 10, 40],
        ),
        (
            msg::model::SongSort::TrackNumber,
            false,
            vec![40, 10, 20, 30],
        ),
        (msg::model::SongSort::Title, true, vec![20, 30, 10, 40]),
    ] {
        let request = msg::QcmMessage {
            r#type: msg::MessageType::GetSongIdsReq.into(),
            payload: Some(msg::qcm_message::Payload::GetSongIdsReq(
                msg::GetSongIdsReq {
                    sort: sort.into(),
                    asc,
                    filters: vec![msg::filter::SongFilter {
                        payload: Some(msg::filter::song_filter::Payload::AlbumIdFilter(
                            msg::filter::AlbumIdFilter { value: 1 },
                        )),
                        ..Default::default()
                    }],
                    ..Default::default()
                },
            )),
            ..Default::default()
        };
        let response = process_qcm(&service, &request.encode_to_vec(), &mut None)
            .await
            .unwrap();
        let Some(msg::qcm_message::Payload::GetSongIdsRsp(response)) = response.payload else {
            panic!("expected song IDs");
        };
        assert_eq!(response.ids, expected, "{sort:?}, ascending: {asc}");
    }

    let request = msg::QcmMessage {
        r#type: msg::MessageType::GetAlbumReq.into(),
        payload: Some(msg::qcm_message::Payload::GetAlbumReq(msg::GetAlbumReq {
            id: 1,
        })),
        ..Default::default()
    };
    let response = process_qcm(&service, &request.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let Some(msg::qcm_message::Payload::GetAlbumRsp(response)) = response.payload else {
        panic!("expected album");
    };
    assert_eq!(response.song_ids, vec![30, 20, 10, 40]);
}
