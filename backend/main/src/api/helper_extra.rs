use crate::convert::QcmInto;
use crate::error::ProcessError;
use crate::msg::{self};
use qcm_core::model::{self as sqlm};
use sea_orm::DatabaseConnection;
use sea_orm::LoaderTrait;

pub fn extra_insert_artists(extra: &mut prost_types::Struct, artists: &[sqlm::artist::Model]) {
    let mut artist_json: Vec<_> = Vec::new();
    for artist in artists {
        artist_json.push(serde_json::json!({
            "id": artist.id.to_string(),
            "name": artist.name,
        }));
    }
    extra.fields.insert(
        "artists".to_string(),
        serde_json::to_string(&artist_json).unwrap().into(),
    );
}

pub fn extra_insert_album(extra: &mut prost_types::Struct, album: &sqlm::album::Model) {
    let j = serde_json::json!({
        "id": album.id.to_string(),
        "name": album.name,
    });
    extra.fields.insert(
        "album".to_string(),
        serde_json::to_string(&j).unwrap().into(),
    );
}

pub fn extra_insert_dynamic(extra: &mut prost_types::Struct, dy: &sqlm::dynamic::Model) {
    let j = serde_json::json!({
        "is_favorite": dy.favorite_at.is_some()
    });
    extra.fields.insert(
        "dynamic".to_string(),
        serde_json::to_string(&j).unwrap().into(),
    );
}

pub async fn to_rsp_songs(
    db: &DatabaseConnection,
    songs: Vec<sqlm::song::Model>,
    album: Option<&sqlm::album::Model>,
) -> Result<(Vec<msg::model::Song>, Vec<prost_types::Struct>), ProcessError> {
    let artists = songs
        .load_many_to_many(sqlm::artist::Entity, sqlm::rel_song_artist::Entity, db)
        .await?;

    let dynamics = songs.load_one(sqlm::dynamic::Entity, db).await?;

    let mut items = Vec::new();
    let mut extras = Vec::new();
    if let Some(album) = album {
        let zip_iter = songs.into_iter().zip(artists).zip(dynamics);
        for ((song, artists), dy) in zip_iter {
            items.push(song.qcm_into());
            let mut extra = prost_types::Struct::default();
            extra_insert_artists(&mut extra, &artists);
            if let Some(dy) = dy {
                extra_insert_dynamic(&mut extra, &dy);
            }
            extra_insert_album(&mut extra, &album);
            extras.push(extra);
        }
    } else {
        let albums = songs.load_one(sqlm::album::Entity, db).await?;
        let zip_iter = songs.into_iter().zip(artists).zip(dynamics).zip(albums);
        for (((song, artists), dy), album) in zip_iter {
            items.push(song.qcm_into());
            let mut extra = prost_types::Struct::default();
            extra_insert_artists(&mut extra, &artists);
            if let Some(dy) = dy {
                extra_insert_dynamic(&mut extra, &dy);
            }
            if let Some(album) = album {
                extra_insert_album(&mut extra, &album);
            }
            extras.push(extra);
        }
    }

    Ok((items, extras))
}

pub async fn to_rsp_albums(
    db: &DatabaseConnection,
    albums: Vec<sqlm::album::Model>,
) -> Result<(Vec<msg::model::Album>, Vec<prost_types::Struct>), ProcessError> {
    let artists = albums
        .load_many_to_many(sqlm::artist::Entity, sqlm::rel_album_artist::Entity, db)
        .await?;

    let dynamics = albums.load_one(sqlm::dynamic::Entity, db).await?;

    let mut items = Vec::new();
    let mut extras = Vec::new();

    for ((album, artists), dy) in albums.into_iter().zip(artists.into_iter()).zip(dynamics) {
        items.push(album.qcm_into());
        let mut extra = prost_types::Struct::default();
        extra_insert_artists(&mut extra, &artists);
        if let Some(dy) = dy {
            extra_insert_dynamic(&mut extra, &dy);
        }
        extras.push(extra);
    }
    Ok((items, extras))
}
