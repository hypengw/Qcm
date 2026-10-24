use crate::{convert::QcmInto, error::ProcessError, event::ServiceContext, msg};
use msg::{entity as e, qcm_message::Payload};
use qcm_core::{db::sync_run::ProjectionRead, model as m};
use sea_orm::{ColumnTrait, EntityTrait, QueryFilter, QueryOrder, TransactionTrait};
use std::collections::{BTreeSet, HashMap};

pub const PROTOCOL: u32 = 1;

#[derive(Default)]
struct References {
    songs: BTreeSet<i64>,
    albums: BTreeSet<i64>,
    artists: BTreeSet<i64>,
    mixes: BTreeSet<i64>,
}
impl References {
    fn payload(&mut self, payload: &Payload) {
        match payload {
            Payload::GetSongsByIdRsp(v) => self.songs.extend(&v.ids),
            Payload::GetAlbumsRsp(v) => self.albums.extend(&v.ids),
            Payload::GetArtistAlbumRsp(v) => self.albums.extend(&v.ids),
            Payload::GetAlbumRsp(v) => {
                self.albums.insert(v.id);
                self.songs.extend(&v.song_ids);
            }
            Payload::GetArtistsRsp(v) => self.artists.extend(&v.ids),
            Payload::GetAlbumArtistsRsp(v) => self.artists.extend(&v.ids),
            Payload::GetArtistRsp(v) => {
                self.artists.insert(v.id);
            }
            Payload::GetMixsRsp(v) => self.mixes.extend(&v.ids),
            Payload::GetRemoteMixsRsp(v) => self.mixes.extend(&v.ids),
            Payload::GetMixRsp(v) => {
                self.mixes.insert(v.id);
            }
            Payload::GetMixSongsRsp(v) => {
                self.mixes.insert(v.mix_id);
                self.songs.extend(&v.ids);
            }
            Payload::CreateMixRsp(v) => {
                self.mixes.insert(v.id);
            }
            Payload::OpenQueueRsp(v) => self.songs.extend(&v.song_ids),
            Payload::GetQueueNextRsp(v) => self.songs.extend(&v.song_ids),
            Payload::SearchRsp(v) => {
                if let Some(v) = &v.songs {
                    self.songs.extend(&v.ids);
                }
                if let Some(v) = &v.albums {
                    self.albums.extend(&v.ids);
                }
                if let Some(v) = &v.artists {
                    self.artists.extend(&v.ids);
                }
            }
            Payload::GetHomeBlockItemsRsp(v) => {
                use msg::get_home_block_items_rsp::Content;
                match &v.content {
                    Some(Content::Songs(v)) => self.songs.extend(&v.ids),
                    Some(Content::Albums(v)) => self.albums.extend(&v.ids),
                    Some(Content::Artists(v)) => self.artists.extend(&v.ids),
                    Some(Content::Mixes(v)) => self.mixes.extend(&v.ids),
                    None => (),
                }
            }
            Payload::GetHomeRecommendationsRsp(v) => {
                use msg::model::for_you_card::Target;
                for provider in &v.providers {
                    for card in &provider.for_you {
                        match &card.target {
                            Some(Target::Song(v)) => {
                                self.songs.insert(v.id);
                            }
                            Some(Target::Album(v)) => {
                                self.albums.insert(v.id);
                            }
                            Some(Target::Artist(v)) => {
                                self.artists.insert(v.id);
                            }
                            Some(Target::Mix(v)) => {
                                self.mixes.insert(v.id);
                            }
                            _ => (),
                        }
                    }
                }
            }
            _ => (),
        }
    }
    fn request(&mut self, payload: &Payload) {
        match payload {
            Payload::SetFavoriteReq(v) => match msg::model::ItemType::try_from(v.item_type) {
                Ok(msg::model::ItemType::Song) => {
                    self.songs.insert(v.id);
                }
                Ok(msg::model::ItemType::Album) => {
                    self.albums.insert(v.id);
                }
                Ok(msg::model::ItemType::Artist | msg::model::ItemType::Albumartist) => {
                    self.artists.insert(v.id);
                }
                _ => (),
            },
            Payload::MixManipulateReq(v) => {
                self.mixes.insert(v.id);
            }
            _ => (),
        }
    }
}

fn text(value: Option<String>) -> Option<e::TextValue> {
    Some(e::TextValue { value })
}
fn time(value: Option<qcm_core::db::values::Timestamp>) -> Option<e::TimeValue> {
    Some(e::TimeValue {
        value: value.map(|value| value.qcm_into()),
    })
}
fn stamp(snapshot: &ProjectionRead, owner: i64) -> Option<e::ReadStamp> {
    Some(e::ReadStamp {
        owner,
        epoch: *snapshot.epochs.get(&owner).unwrap_or(&0),
        revision: snapshot.revision,
    })
}

pub async fn query(
    ctx: &ServiceContext,
    request: &msg::QcmMessage,
) -> Result<msg::QcmMessage, ProcessError> {
    let snapshot = ctx.provider_context.sync_coordinator.read_snapshot().await;
    let db = ctx.provider_context.db.begin().await?;
    let mut response = super::entity_query::execute(&db, request).await?;
    populate_at(request, &mut response, &snapshot, &db).await?;
    db.commit().await?;
    Ok(response)
}

pub async fn populate(
    ctx: &ServiceContext,
    request: &msg::QcmMessage,
    response: &mut msg::QcmMessage,
) -> Result<(), ProcessError> {
    let snapshot = ctx.provider_context.sync_coordinator.read_snapshot().await;
    let db = ctx.provider_context.db.begin().await?;
    populate_at(request, response, &snapshot, &db).await?;
    db.commit().await?;
    Ok(())
}

async fn populate_at(
    request: &msg::QcmMessage,
    response: &mut msg::QcmMessage,
    snapshot: &ProjectionRead,
    db: &sea_orm::DatabaseTransaction,
) -> Result<(), ProcessError> {
    let mut refs = References::default();
    if let Some(payload) = &response.payload {
        refs.payload(payload);
    }
    if let Some(payload) = &request.payload {
        refs.request(payload);
        match payload {
            Payload::DeleteMixReq(v) => refs.mixes.extend(&v.ids),
            Payload::LinkMixReq(v) => refs.mixes.extend(&v.ids),
            _ => (),
        }
    }
    let mut batch = e::EntityBatch {
        protocol: PROTOCOL,
        instance: snapshot.instance.clone(),
        ..Default::default()
    };
    for (&owner, &epoch) in &snapshot.epochs {
        batch.invalidations.push(e::ReadStamp {
            owner,
            epoch,
            revision: snapshot.revision,
        });
    }
    if refs.songs.is_empty()
        && refs.albums.is_empty()
        && refs.artists.is_empty()
        && refs.mixes.is_empty()
    {
        response.entities = Some(batch);
        return Ok(());
    }
    let songs = m::song::Entity::find()
        .filter(m::song::Column::Id.is_in(refs.songs.iter().copied()))
        .all(db)
        .await?;
    for song in &songs {
        refs.albums.extend(song.album_id);
    }
    let song_artists = m::rel_song_artist::Entity::find()
        .filter(m::rel_song_artist::Column::SongId.is_in(refs.songs.iter().copied()))
        .order_by_asc(m::rel_song_artist::Column::Id)
        .all(db)
        .await?;
    let album_artists = m::rel_album_artist::Entity::find()
        .filter(m::rel_album_artist::Column::AlbumId.is_in(refs.albums.iter().copied()))
        .order_by_asc(m::rel_album_artist::Column::Id)
        .all(db)
        .await?;
    refs.artists
        .extend(song_artists.iter().map(|rel| rel.artist_id));
    refs.artists
        .extend(album_artists.iter().map(|rel| rel.artist_id));
    let albums = m::album::Entity::find()
        .filter(m::album::Column::Id.is_in(refs.albums.iter().copied()))
        .all(db)
        .await?;
    let artists = m::artist::Entity::find()
        .filter(m::artist::Column::Id.is_in(refs.artists.iter().copied()))
        .all(db)
        .await?;
    let mixes = m::mix::Entity::find()
        .filter(m::mix::Column::Id.is_in(refs.mixes.iter().copied()))
        .all(db)
        .await?;
    let ids: Vec<_> = refs
        .songs
        .iter()
        .chain(&refs.albums)
        .chain(&refs.artists)
        .copied()
        .collect();
    let items = m::item::Entity::find()
        .filter(m::item::Column::Id.is_in(ids.clone()))
        .all(db)
        .await?;
    let owners: HashMap<_, _> = items
        .into_iter()
        .map(|item| (item.id, item.provider_id))
        .collect();
    let dynamics: HashMap<_, _> = m::dynamic::Entity::find()
        .filter(m::dynamic::Column::Id.is_in(ids))
        .all(db)
        .await?
        .into_iter()
        .map(|item| (item.id, item.favorite_at.is_some()))
        .collect();
    let owner = |id| owners.get(&id).copied().ok_or(ProcessError::NotFound);
    let state = |id| {
        Some(e::ItemState {
            favorite: Some(*dynamics.get(&id).unwrap_or(&false)),
        })
    };
    for song in songs {
        refs.songs.remove(&song.id);
        batch.songs.push(e::SongUpdate {
            id: song.id,
            stamp: stamp(snapshot, owner(song.id)?),
            fields: Some(e::SongFields {
                name: Some(song.name),
                sort_name: text(song.sort_name),
                track_number: Some(song.track_number),
                disc_number: Some(song.disc_number),
                duration: Some(song.duration as f64),
                can_play: Some(song.can_play),
                publish_time: time(song.publish_time),
                popularity: Some(song.popularity),
                tags: Some(e::StringList {
                    values: song
                        .tags
                        .as_array()
                        .into_iter()
                        .flatten()
                        .filter_map(|tag| tag.as_str().map(String::from))
                        .collect(),
                }),
            }),
            relations: Some(e::SongRelations {
                album: Some(e::AlbumLink { id: song.album_id }),
                artists: Some(e::EntityIds {
                    ids: song_artists
                        .iter()
                        .filter(|rel| rel.song_id == song.id)
                        .map(|rel| rel.artist_id)
                        .collect(),
                }),
            }),
            state: state(song.id),
        });
    }
    for album in albums {
        refs.albums.remove(&album.id);
        batch.albums.push(e::AlbumUpdate {
            id: album.id,
            stamp: stamp(snapshot, owner(album.id)?),
            fields: Some(e::AlbumFields {
                name: Some(album.name),
                sort_name: text(album.sort_name),
                r#type: Some(album.r#type as i32),
                duration: Some(album.duration),
                track_count: Some(album.track_count),
                disc_count: Some(album.disc_count),
                description: text(album.description),
                company: text(album.company),
                language: text(album.language),
                publish_time: time(album.publish_time),
                added_at: time(album.added_at),
            }),
            relations: Some(e::AlbumRelations {
                artists: Some(e::EntityIds {
                    ids: album_artists
                        .iter()
                        .filter(|rel| rel.album_id == album.id)
                        .map(|rel| rel.artist_id)
                        .collect(),
                }),
            }),
            state: state(album.id),
        });
    }
    for artist in artists {
        refs.artists.remove(&artist.id);
        batch.artists.push(e::ArtistUpdate {
            id: artist.id,
            stamp: stamp(snapshot, owner(artist.id)?),
            fields: Some(e::ArtistFields {
                name: Some(artist.name),
                sort_name: text(artist.sort_name),
                description: Some(artist.description),
                album_count: Some(artist.album_count),
                music_count: Some(artist.music_count),
                added_at: time(artist.added_at),
            }),
            state: state(artist.id),
        });
    }
    for mix in mixes {
        refs.mixes.remove(&mix.id);
        batch.mixes.push(e::MixUpdate {
            id: mix.id,
            stamp: stamp(snapshot, 0),
            fields: Some(e::MixFields {
                name: Some(mix.name),
                sort_name: text(mix.sort_name),
                track_count: Some(mix.track_count),
                description: Some(mix.description),
                mix_type: Some(mix.mix_type as i32),
                update_at: time(Some(mix.update_at)),
            }),
        });
    }
    for (kind, ids) in [
        (msg::model::ItemType::Song, refs.songs),
        (msg::model::ItemType::Album, refs.albums),
        (msg::model::ItemType::Artist, refs.artists),
        (msg::model::ItemType::Mix, refs.mixes),
    ] {
        for id in ids {
            batch.removals.push(e::Removal {
                id,
                r#type: kind.into(),
                stamp: stamp(snapshot, 0),
            });
        }
    }
    response.entities = Some(batch);
    Ok(())
}
