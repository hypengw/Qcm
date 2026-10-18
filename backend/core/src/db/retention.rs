use super::sync_run::SyncCoordinator;
use crate::model as m;
use sea_orm::{ColumnTrait, DatabaseConnection, DbErr, EntityTrait, QueryFilter, QuerySelect};
use std::{
    collections::{HashMap, HashSet},
    sync::{Arc, Mutex},
};

const MAX_ROOT_ITEMS: usize = 4096;

#[derive(Debug, Default)]
pub(super) struct RetainedItems(pub Mutex<HashMap<i64, usize>>);

/// Protects item identities and media dependencies from full-sync pruning until dropped.
pub struct ItemRetention {
    owner: Arc<RetainedItems>,
    roots: Vec<m::item::Model>,
    ids: HashSet<i64>,
}

impl ItemRetention {
    pub fn roots(&self) -> &[m::item::Model] {
        &self.roots
    }
}

impl Drop for ItemRetention {
    fn drop(&mut self) {
        let mut counts = self.owner.0.lock().unwrap();
        for id in &self.ids {
            if let Some(count) = counts.get_mut(id) {
                *count -= 1;
                if *count == 0 {
                    counts.remove(id);
                }
            }
        }
    }
}

impl SyncCoordinator {
    pub async fn retain_items(
        &self,
        db: &DatabaseConnection,
        provider: i64,
        ids: impl IntoIterator<Item = i64>,
    ) -> Result<ItemRetention, DbErr> {
        let state = self.state(provider);
        // Retention and pruning share serialization without changing the sync write revision.
        let _write = state.writes.clone().lock_owned().await;
        let mut ids: HashSet<_> = ids.into_iter().collect();
        if ids.len() > MAX_ROOT_ITEMS || ids.iter().any(|id| *id <= 0) {
            return Err(DbErr::Custom("invalid retained item set".into()));
        }
        let roots = m::item::Entity::find()
            .filter(m::item::Column::ProviderId.eq(provider))
            .filter(m::item::Column::Id.is_in(ids.iter().copied()))
            .all(db)
            .await?;
        if roots.len() != ids.len() {
            return Err(DbErr::Custom(
                "retained item is missing or belongs to another provider".into(),
            ));
        }
        let albums: Vec<Option<i64>> = m::song::Entity::find()
            .select_only()
            .column(m::song::Column::AlbumId)
            .filter(m::song::Column::Id.is_in(ids.iter().copied()))
            .into_tuple()
            .all(db)
            .await?;
        ids.extend(albums.into_iter().flatten());
        let song_artists: Vec<i64> = m::rel_song_artist::Entity::find()
            .select_only()
            .column(m::rel_song_artist::Column::ArtistId)
            .filter(m::rel_song_artist::Column::SongId.is_in(ids.iter().copied()))
            .into_tuple()
            .all(db)
            .await?;
        let album_artists: Vec<i64> = m::rel_album_artist::Entity::find()
            .select_only()
            .column(m::rel_album_artist::Column::ArtistId)
            .filter(m::rel_album_artist::Column::AlbumId.is_in(ids.iter().copied()))
            .into_tuple()
            .all(db)
            .await?;
        ids.extend(song_artists);
        ids.extend(album_artists);
        let dependencies: Vec<i64> = m::item::Entity::find()
            .select_only()
            .column(m::item::Column::Id)
            .filter(m::item::Column::ProviderId.eq(provider))
            .filter(m::item::Column::Id.is_in(ids.iter().copied()))
            .into_tuple()
            .all(db)
            .await?;
        if dependencies.len() != ids.len() {
            return Err(DbErr::Custom(
                "retained item dependency is missing or belongs to another provider".into(),
            ));
        }
        {
            let mut counts = state.retained.0.lock().unwrap();
            for id in &ids {
                *counts.entry(*id).or_default() += 1;
            }
        }
        Ok(ItemRetention {
            owner: state.retained.clone(),
            roots,
            ids,
        })
    }
}
