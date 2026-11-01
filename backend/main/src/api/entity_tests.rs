use super::process_qcm::process_qcm;
use crate::{event::ServiceContext, msg};
use prost::Message;
use qcm_core::{model as m, provider::Context};
use sea_orm::{ConnectionTrait, Database, EntityTrait, Schema};
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
    let (backend_ev, mut backend_events) = tokio::sync::mpsc::channel(1);
    let (oper, _) = crate::task::TaskManager::new();
    let service = Arc::new(ServiceContext {
        protocol: Arc::new(Default::default()),
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
    let hello = msg::QcmMessage {
        protocol_version: crate::protocol::VERSION,
        id: 11,
        r#type: msg::MessageType::TestReq as i32,
        payload: Some(msg::qcm_message::Payload::TestReq(Default::default())),
        ..Default::default()
    };
    check_protocol_transports(&service).await;
    let (sender, mut replies) = tokio::sync::mpsc::channel(2);
    super::handler::handle_ws_message(
        super::process_ws::WsMessage::Binary(hello.encode_to_vec().into()),
        service.clone(),
        sender.clone(),
    )
    .await
    .unwrap();
    let response =
        msg::QcmMessage::decode(replies.recv().await.unwrap().into_data().as_ref()).unwrap();
    assert_eq!(response.protocol_version, crate::protocol::VERSION);
    assert_eq!(
        response.entities.as_ref().unwrap().protocol,
        crate::protocol::VERSION
    );
    assert!(service.protocol.ready());
    assert!(matches!(
        backend_events.try_recv().unwrap(),
        crate::event::BackendEvent::Frist
    ));
    super::handler::handle_ws_message(
        super::process_ws::WsMessage::Binary(hello.encode_to_vec().into()),
        service.clone(),
        sender,
    )
    .await
    .unwrap();
    assert!(replies.recv().await.unwrap().is_binary());
    assert!(backend_events.try_recv().is_err());
    check_artist_roles(&service).await;
    let request = msg::QcmMessage {
        protocol_version: crate::protocol::VERSION,
        r#type: msg::MessageType::GetSongsByIdReq as i32,
        payload: Some(msg::qcm_message::Payload::GetSongsByIdReq(
            msg::GetSongsByIdReq {
                ids: Some(vec![SONG, 99].into()),
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
    assert_eq!(batch.protocol, crate::protocol::VERSION);
    assert!(!batch.instance.is_empty());
    let song = &batch.songs[0];
    assert_eq!(song.id.as_ref().unwrap().value, SONG);
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
        Some(2.into())
    );
    assert_eq!(
        song.relations
            .as_ref()
            .unwrap()
            .artists
            .as_ref()
            .unwrap()
            .values,
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
            .values,
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
        .values
        .is_empty());
    assert_eq!(song.state.as_ref().unwrap().favorite, Some(false));
    cache.shutdown().await.unwrap();
    std::fs::remove_dir_all(directory).unwrap();
}

async fn check_artist_roles(service: &Arc<ServiceContext>) {
    use msg::qcm_message::Payload;
    let db = &service.provider_context.db;
    db.execute_unprepared("INSERT INTO item(id,native_id,provider_id,type,create_at,update_at,last_sync_at) VALUES(4,'album-artist',7,53,0,0,0);
        INSERT INTO artist(id,name,description,album_count,music_count) VALUES(4,'Album Artist','',0,0);
        UPDATE item SET library_id=8;
        UPDATE rel_album_artist SET artist_id=4;
        INSERT INTO dynamic(id,is_external,play_count,remote_play_count,update_at) VALUES(2,0,0,0,0);").await.unwrap();
    let request = |kind, payload| msg::QcmMessage {
        protocol_version: crate::protocol::VERSION,
        r#type: kind as i32,
        payload: Some(payload),
        ..Default::default()
    };
    let query = request(
        msg::MessageType::GetArtistsReq,
        Payload::GetArtistsReq(msg::GetArtistsReq {
            library_id: Some(vec![8].into()),
            page_size: 20,
            ..Default::default()
        }),
    );
    let response = process_qcm(service, &query.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let Some(Payload::GetArtistsRsp(response)) = response.payload else {
        panic!("artists");
    };
    assert_eq!(crate::identity::IdList::values(&response.ids), &[3]);
    let query = request(
        msg::MessageType::GetAlbumArtistsReq,
        Payload::GetAlbumArtistsReq(msg::GetAlbumArtistsReq {
            library_id: Some(vec![8].into()),
            page_size: 20,
            ..Default::default()
        }),
    );
    let response = process_qcm(service, &query.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let Some(Payload::GetAlbumArtistsRsp(response)) = response.payload else {
        panic!("album artists");
    };
    assert_eq!(crate::identity::IdList::values(&response.ids), &[4]);
    for (album_role, artist, expected) in
        [(true, 3, vec![]), (false, 3, vec![2]), (true, 4, vec![2])]
    {
        let payload = if album_role {
            msg::filter::album_filter::Payload::AlbumArtistIdFilter(
                msg::filter::AlbumArtistIdFilter {
                    value: Some(artist.into()),
                },
            )
        } else {
            msg::filter::album_filter::Payload::ArtistIdFilter(msg::filter::ArtistIdFilter {
                value: Some(artist.into()),
            })
        };
        let query = request(
            msg::MessageType::GetAlbumsReq,
            Payload::GetAlbumsReq(msg::GetAlbumsReq {
                library_id: Some(vec![8].into()),
                page_size: 20,
                filters: vec![msg::filter::AlbumFilter {
                    payload: Some(payload),
                    ..Default::default()
                }],
                ..Default::default()
            }),
        );
        let response = process_qcm(service, &query.encode_to_vec(), &mut None)
            .await
            .unwrap();
        let Some(Payload::GetAlbumsRsp(response)) = response.payload else {
            panic!("albums");
        };
        assert_eq!(crate::identity::IdList::values(&response.ids), expected);
    }
    let query = request(
        msg::MessageType::GetArtistReq,
        Payload::GetArtistReq(msg::GetArtistReq { id: Some(3.into()) }),
    );
    let response = process_qcm(service, &query.encode_to_vec(), &mut None)
        .await
        .unwrap();
    let Some(Payload::GetArtistRsp(response)) = response.payload else {
        panic!("artist");
    };
    assert_eq!(response.id.unwrap().value, 3);
    let query = request(
        msg::MessageType::SetFavoriteReq,
        Payload::SetFavoriteReq(msg::SetFavoriteReq {
            id: 2,
            item_type: msg::model::ItemType::Song as i32,
            value: true,
        }),
    );
    assert!(matches!(
        process_qcm(service, &query.encode_to_vec(), &mut None).await,
        Err(crate::error::ProcessError::WrongId(_))
    ));
    let query = request(
        msg::MessageType::PlaylogReq,
        Payload::PlaylogReq(msg::PlaylogReq {
            song_id: Some(2.into()),
            ..Default::default()
        }),
    );
    assert!(matches!(
        process_qcm(service, &query.encode_to_vec(), &mut None).await,
        Err(crate::error::ProcessError::NoSuchSong(_))
    ));
    db.execute_unprepared("UPDATE rel_album_artist SET artist_id=3; DELETE FROM artist WHERE id=4; DELETE FROM item WHERE id=4; DELETE FROM dynamic WHERE id=2;").await.unwrap();
}

async fn check_protocol_transports(service: &Arc<ServiceContext>) {
    use hyper_util::rt::TokioIo;
    use sea_orm::PaginatorTrait;
    let request = |version| msg::QcmMessage {
        protocol_version: version,
        id: 71,
        r#type: msg::MessageType::CreateMixReq as i32,
        payload: Some(msg::qcm_message::Payload::CreateMixReq(msg::CreateMixReq {
            name: "must not be created".into(),
            ..Default::default()
        })),
        ..Default::default()
    };
    let versions = [0, 1, 3, crate::protocol::VERSION];
    let (sender, mut replies) = tokio::sync::mpsc::channel(1);
    for version in versions {
        super::handler::handle_ws_message(
            super::process_ws::WsMessage::Binary(request(version).encode_to_vec().into()),
            service.clone(),
            sender.clone(),
        )
        .await
        .unwrap();
        let response =
            msg::QcmMessage::decode(replies.recv().await.unwrap().into_data().as_ref()).unwrap();
        assert_eq!(response.protocol_version, crate::protocol::VERSION);
        assert_eq!(response.id, 71);
        let Some(msg::qcm_message::Payload::Rsp(error)) = response.payload else {
            panic!("expected WS error");
        };
        assert_eq!(
            error.code,
            if version == crate::protocol::VERSION {
                msg::ErrorCode::ProtocolNotReady
            } else {
                msg::ErrorCode::ProtocolVersion
            } as i32
        );
    }
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await.unwrap();
    let address = listener.local_addr().unwrap();
    let server = tokio::spawn({
        let service = service.clone();
        async move {
            for _ in versions {
                let (socket, _) = listener.accept().await.unwrap();
                let service = service.clone();
                hyper::server::conn::http1::Builder::new()
                    .serve_connection(
                        TokioIo::new(socket),
                        hyper::service::service_fn(move |request| {
                            let service = service.clone();
                            async move {
                                super::process_http::process_http_post(&service, request).await
                            }
                        }),
                    )
                    .await
                    .unwrap();
            }
        }
    });
    let client = reqwest::Client::new();
    for version in versions {
        let response = client
            .post(format!("http://{address}/"))
            .header("Connection", "close")
            .body(request(version).encode_to_vec())
            .send()
            .await
            .unwrap();
        let bytes = response.bytes().await.unwrap();
        let response = msg::QcmMessage::decode(bytes.as_ref()).unwrap();
        assert_eq!(response.protocol_version, crate::protocol::VERSION);
        assert_eq!(response.id, 71);
        let Some(msg::qcm_message::Payload::Rsp(error)) = response.payload else {
            panic!("expected HTTP error");
        };
        assert_eq!(
            error.code,
            if version == crate::protocol::VERSION {
                msg::ErrorCode::ProtocolNotReady
            } else {
                msg::ErrorCode::ProtocolVersion
            } as i32
        );
    }
    server.await.unwrap();
    assert!(!service.protocol.ready());
    assert_eq!(
        m::mix::Entity::find()
            .count(&service.provider_context.db)
            .await
            .unwrap(),
        0
    );
}
