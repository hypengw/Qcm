use super::process_qcm::process_qcm;
use crate::{event::ServiceContext, msg};
use prost::Message;
use qcm_core::{model as m, provider::Context};
use sea_orm::{ConnectionTrait, Database, Schema};
use std::sync::Arc;

#[tokio::test]
async fn entity_projection_keeps_presence_relations_and_authoritative_state() {
    let db = Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("PRAGMA foreign_keys=OFF")
        .await
        .unwrap();
    let schema = Schema::new(db.get_database_backend());
    for table in [
        schema.create_table_from_entity(m::song::Entity),
        schema.create_table_from_entity(m::album::Entity),
        schema.create_table_from_entity(m::artist::Entity),
        schema.create_table_from_entity(m::mix::Entity),
        schema.create_table_from_entity(m::item::Entity),
        schema.create_table_from_entity(m::dynamic::Entity),
        schema.create_table_from_entity(m::rel_song_artist::Entity),
        schema.create_table_from_entity(m::rel_album_artist::Entity),
    ] {
        db.execute(db.get_database_backend().build(&table))
            .await
            .unwrap();
    }
    const SONG: i64 = 9007199254740993;
    db.execute_unprepared(&format!("INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at) VALUES
        ({SONG},'song',7,101,0,0,0),(2,'album',7,51,0,0,0),(3,'artist',7,53,0,0,0);
        INSERT INTO album(id,name,track_count,disc_count,type,duration) VALUES(2,'Album',0,0,0,0);
        INSERT INTO artist(id,name,description,album_count,music_count) VALUES(3,'Artist','detail',0,0);
        INSERT INTO song(id,name,album_id,disc_number,track_number,duration,can_play,popularity,tags) VALUES({SONG},'',2,0,0,0,0,0,'[]');
        INSERT INTO rel_song_artist(id,song_id,artist_id,update_at) VALUES(1,{SONG},3,0);
        INSERT INTO rel_album_artist(id,album_id,artist_id,update_at) VALUES(1,2,3,0);")).await.unwrap();
    let directory = std::env::temp_dir().join(format!("qcm-entity-test-{}", uuid::Uuid::new_v4()));
    let cache = crate::resource_cache::ResourceCache::open(directory.clone())
        .await
        .unwrap();
    let (events, _) = tokio::sync::mpsc::channel(1);
    let (backend_ev, _) = tokio::sync::mpsc::channel(1);
    let (oper, _) = crate::task::TaskManager::new();
    let service = Arc::new(ServiceContext {
        provider_context: Arc::new(Context {
            db: db.clone(),
            cache_db: db.clone(),
            ev_sender: events,
            sync_coordinator: Default::default(),
        }),
        backend_ev,
        oper,
        mix_refresh: Default::default(),
        resource_cache: cache.clone(),
        radio: Default::default(),
        playback: Default::default(),
    });
    let request = msg::QcmMessage {
        r#type: msg::MessageType::GetSongsByIdReq as i32,
        payload: Some(msg::qcm_message::Payload::GetSongsByIdReq(
            msg::GetSongsByIdReq {
                ids: vec![SONG, 99],
            },
        )),
        ..Default::default()
    };
    let first = process_qcm(&service, &request.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let bytes = first.encode_to_vec();
    let decoded = msg::QcmMessage::decode(bytes.as_slice()).unwrap();
    let batch = decoded.entities.unwrap();
    assert_eq!(batch.protocol, 1);
    assert!(!batch.instance.is_empty());
    let song = &batch.songs[0];
    assert_eq!(song.id, SONG);
    assert_eq!(song.stamp.as_ref().unwrap().owner, 7);
    let fields = song.fields.as_ref().unwrap();
    assert_eq!(fields.name, Some(String::new()));
    assert_eq!(fields.can_play, Some(false));
    assert_eq!(fields.track_number, Some(0));
    assert_eq!(fields.sort_name.as_ref().unwrap().value, None);
    assert_eq!(fields.publish_time.as_ref().unwrap().value, None);
    assert!(fields.tags.as_ref().unwrap().values.is_empty());
    assert_eq!(
        song.relations.as_ref().unwrap().album.as_ref().unwrap().id,
        Some(2)
    );
    assert_eq!(
        song.relations
            .as_ref()
            .unwrap()
            .artists
            .as_ref()
            .unwrap()
            .ids,
        vec![3]
    );
    assert_eq!(song.state.as_ref().unwrap().favorite, Some(false));
    assert_eq!(
        batch.albums[0]
            .relations
            .as_ref()
            .unwrap()
            .artists
            .as_ref()
            .unwrap()
            .ids,
        vec![3]
    );
    assert_eq!(
        batch.artists[0].fields.as_ref().unwrap().description,
        Some("detail".into())
    );
    assert_eq!(batch.removals[0].id, 99);
    let write = service
        .provider_context
        .sync_coordinator
        .write(7, None)
        .await
        .unwrap();
    db.execute_unprepared(&format!("INSERT INTO dynamic(id,is_external,play_count,remote_play_count,favorite_at,update_at) VALUES({SONG},0,0,0,1,0); UPDATE song SET album_id=NULL WHERE id={SONG}; DELETE FROM rel_song_artist WHERE song_id={SONG};")).await.unwrap();
    drop(write);
    let second = process_qcm(&service, &request.encode_to_vec(), &mut None)
        .await
        .unwrap()
        .entities
        .unwrap();
    let updated = &second.songs[0];
    assert!(updated.stamp.as_ref().unwrap().revision > song.stamp.as_ref().unwrap().revision);
    assert_eq!(updated.state.as_ref().unwrap().favorite, Some(true));
    assert_eq!(
        updated
            .relations
            .as_ref()
            .unwrap()
            .album
            .as_ref()
            .unwrap()
            .id,
        None
    );
    assert!(updated
        .relations
        .as_ref()
        .unwrap()
        .artists
        .as_ref()
        .unwrap()
        .ids
        .is_empty());
    assert_eq!(song.state.as_ref().unwrap().favorite, Some(false));
    cache.shutdown().await.unwrap();
    std::fs::remove_dir_all(directory).unwrap();
}
