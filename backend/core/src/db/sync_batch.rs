use super::values::Timestamp;
use crate::model::{
    self as m,
    type_enum::{AlbumType, ImageType, ItemType, MixType},
};
use sea_orm::*;
use serde::{Deserialize, Deserializer, Serialize};
use std::collections::HashSet;

#[derive(Clone, Debug, PartialEq, Serialize)]
pub enum Patch<T> {
    Missing,
    Set(T),
}
impl<T> Default for Patch<T> {
    fn default() -> Self {
        Self::Missing
    }
}
impl<'de, T: Deserialize<'de>> Deserialize<'de> for Patch<T> {
    fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        T::deserialize(deserializer).map(Self::Set)
    }
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
pub struct ItemKey {
    pub native_id: String,
    #[serde(default)]
    pub library_id: Patch<Option<i64>>,
    pub r#type: ItemType,
    pub id: Option<i64>,
}
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
#[serde(untagged)]
pub enum ItemRef {
    Id(i64),
    Key(ItemKey),
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Mask {
    pub include: Option<Vec<String>>,
    pub exclude: Option<Vec<String>>,
}
impl Mask {
    fn validate(&self, fields: &[&str]) -> Result<(), DbErr> {
        if self.include.is_some() && self.exclude.is_some() {
            return Err(invalid("include and exclude are mutually exclusive"));
        }
        for name in self.include.iter().chain(self.exclude.iter()).flatten() {
            if !fields.contains(&name.as_str()) {
                return Err(invalid(format!("unknown mask field: {name}")));
            }
        }
        Ok(())
    }
    fn allows(&self, name: &str) -> bool {
        self.include
            .as_ref()
            .is_none_or(|v| v.iter().any(|n| n == name))
            && self
                .exclude
                .as_ref()
                .is_none_or(|v| !v.iter().any(|n| n == name))
    }
}
fn invalid(message: impl Into<String>) -> DbErr {
    DbErr::Custom(message.into())
}

macro_rules! entity_input {
    ($input:ident, $fields:ident, $model:ident, {$($field:ident: $ty:ty),* $(,)?} $(, {$($extra:ident: $extra_ty:ty),* $(,)?})?) => {
        #[derive(Clone, Debug, Default, Serialize, Deserialize)]
        #[serde(deny_unknown_fields)]
        pub struct $fields { $(#[serde(default)] pub $field: Patch<$ty>,)* }
        #[derive(Clone, Debug, Serialize, Deserialize)]
        #[serde(deny_unknown_fields)]
        pub struct $input {
            #[serde(alias="id")] pub item: ItemRef,
            pub on_insert: Option<$fields>,
            $(#[serde(default)] pub $field: Patch<$ty>,)*
            $($(#[serde(default)] pub $extra: $extra_ty,)*)?
        }
        impl $input {
            fn validate_mask(mask: &Mask) -> Result<(), DbErr> {
                mask.validate(&[$(stringify!($field).trim_start_matches("r#"),)* $($(stringify!($extra),)*)?])
            }
            fn apply(&self, model: &mut m::$model::Model, inserted: bool, mask: &Mask) -> Result<(), DbErr> {
                mask.validate(&[$(stringify!($field).trim_start_matches("r#"),)* $($(stringify!($extra),)*)?])?;
                $(
                    if inserted {
                        if let Some(defaults) = &self.on_insert {
                            if let Patch::Set(value) = &defaults.$field { model.$field = value.clone().into(); }
                        }
                    }
                    if inserted || mask.allows(stringify!($field).trim_start_matches("r#")) {
                        if let Patch::Set(value) = &self.$field { model.$field = value.clone().into(); }
                    }
                )*
                Ok(())
            }
        }
    };
}
entity_input!(AlbumInput, AlbumFields, album, {
    name: String, sort_name: Option<String>, track_count: i32, disc_count: i32,
    r#type: AlbumType, duration: i64, language: Option<String>, description: Option<String>,
    company: Option<String>, publish_time: Option<Timestamp>, added_at: Option<Timestamp>,
});
entity_input!(ArtistInput, ArtistFields, artist, {
    name: String, sort_name: Option<String>, description: String, album_count: i32,
    music_count: i32, added_at: Option<Timestamp>,
});
entity_input!(SongInput, SongFields, song, {
    name: String, sort_name: Option<String>, track_number: i32, disc_number: i32,
    duration: i64, can_play: bool, popularity: f64, tags: Vec<String>,
    publish_time: Option<Timestamp>, added_at: Option<Timestamp>,
}, { album_id: Patch<Option<ItemRef>> });
entity_input!(RemoteMixInput, RemoteMixFields, remote_mix, {
    name: String, description: Option<String>, track_count: i32, mix_type: String,
});
entity_input!(DynamicInput, DynamicFields, dynamic, {
    is_external: bool, remote_play_count: i64, remote_last_played_at: Option<Timestamp>,
    favorite_at: Option<Timestamp>,
}, { favorite: Patch<bool> });

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct LibraryInput {
    pub native_id: String,
    pub name: String,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct ImageInput {
    pub item_id: ItemRef,
    pub image_type: ImageType,
    #[serde(default)]
    pub native_id: Patch<Option<String>>,
    #[serde(default)]
    pub fresh: Patch<String>,
    #[serde(default)]
    pub remove: bool,
}
#[derive(Clone, Copy, Debug, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum RelationMode {
    Merge,
    Replace,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct RelationInput {
    pub owner: ItemRef,
    pub mode: RelationMode,
    pub targets: Vec<ItemRef>,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct SongAlbumInput {
    pub song: ItemRef,
    pub album: Option<ItemRef>,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct MixContents {
    pub mix: ItemRef,
    pub mode: RelationMode,
    pub songs: Vec<ItemRef>,
}
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct BatchOptions {
    #[serde(default)]
    pub albums: Mask,
    #[serde(default)]
    pub artists: Mask,
    #[serde(default)]
    pub songs: Mask,
    #[serde(default)]
    pub remote_mixes: Mask,
    #[serde(default)]
    pub dynamics: Mask,
}
#[derive(Clone, Debug, Default, Serialize, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct SyncBatch {
    pub libraries: Vec<LibraryInput>,
    pub albums: Vec<AlbumInput>,
    pub artists: Vec<ArtistInput>,
    pub songs: Vec<SongInput>,
    pub remote_mixes: Vec<RemoteMixInput>,
    pub dynamics: Vec<DynamicInput>,
    pub images: Vec<ImageInput>,
    pub song_albums: Vec<SongAlbumInput>,
    pub album_artists: Vec<RelationInput>,
    pub song_artists: Vec<RelationInput>,
    pub remote_mix_contents: Vec<MixContents>,
    pub options: BatchOptions,
}
#[derive(Debug, Default, Serialize)]
pub struct SyncResult {
    pub libraries: Vec<m::library::Model>,
    pub items: Vec<m::item::Model>,
    pub albums: Vec<m::album::Model>,
    pub artists: Vec<m::artist::Model>,
    pub songs: Vec<m::song::Model>,
    pub remote_mixes: Vec<m::remote_mix::Model>,
}

pub struct ProviderWriter<'a> {
    db: &'a DatabaseConnection,
    provider_id: i64,
    coordinator: &'a super::sync_run::SyncCoordinator,
    run: Option<(u64, u64)>,
    generation: Option<super::sync_run::WriteGeneration>,
}
impl<'a> ProviderWriter<'a> {
    pub fn new(
        db: &'a DatabaseConnection,
        provider_id: i64,
        coordinator: &'a super::sync_run::SyncCoordinator,
    ) -> Self {
        Self {
            db,
            provider_id,
            coordinator,
            run: None,
            generation: None,
        }
    }
    pub fn with_generation(mut self, generation: Option<super::sync_run::WriteGeneration>) -> Self {
        self.generation = generation;
        self
    }
    pub fn from_run(mut self, token: u64, revision: u64) -> Self {
        self.run = Some((token, revision));
        self
    }
    pub async fn resolve(
        &self,
        txn: &DatabaseTransaction,
        item: &ItemRef,
        expected: &[ItemType],
    ) -> Result<m::item::Model, DbErr> {
        self.resolve_optional(txn, item, expected).await?
            .ok_or_else(|| invalid(format!("missing item reference: {item:?}")))
    }
    pub async fn resolve_optional(
        &self,
        txn: &DatabaseTransaction,
        item: &ItemRef,
        expected: &[ItemType],
    ) -> Result<Option<m::item::Model>, DbErr> {
        let model = match item {
            ItemRef::Id(id) => m::item::Entity::find_by_id(*id).one(txn).await?,
            ItemRef::Key(key) => {
                if !expected.is_empty() && !expected.contains(&key.r#type) {
                    return Err(invalid("item reference has wrong type"));
                }
                let Some(library) = self.library_optional(txn, key).await? else { return Ok(None) };
                let mut query = m::item::Entity::find()
                    .filter(m::item::Column::ProviderId.eq(self.provider_id))
                    .filter(m::item::Column::NativeId.eq(&key.native_id))
                    .filter(m::item::Column::Type.eq(key.r#type));
                query = match library {
                    Some(id) => query.filter(m::item::Column::LibraryId.eq(id)),
                    None => query.filter(m::item::Column::LibraryId.is_null()),
                };
                let found = query.one(txn).await?;
                if let (Some(id), Some(model)) = (key.id, found.as_ref()) {
                    if id != model.id {
                        return Err(invalid("item id does not match key"));
                    }
                }
                found
            }
        };
        let Some(model) = model else { return Ok(None) };
        if model.provider_id != self.provider_id
            || (!expected.is_empty() && !expected.contains(&model.r#type))
        {
            return Err(invalid("item reference has wrong provider or type"));
        }
        Ok(Some(model))
    }
    pub async fn resolve_unique(
        &self,
        native_id: &str,
        kind: ItemType,
    ) -> Result<m::item::Model, DbErr> {
        let mut matches = m::item::Entity::find()
            .filter(m::item::Column::ProviderId.eq(self.provider_id))
            .filter(m::item::Column::NativeId.eq(native_id))
            .filter(m::item::Column::Type.eq(kind))
            .limit(2)
            .all(self.db)
            .await?;
        if matches.len() != 1 {
            return Err(invalid(if matches.is_empty() {
                "missing item reference"
            } else {
                "ambiguous item reference; supply a library"
            }));
        }
        Ok(matches.remove(0))
    }
    async fn owns_library(&self, txn: &DatabaseTransaction, id: i64) -> Result<bool, DbErr> {
        let Some(library) = m::library::Entity::find_by_id(id).one(txn).await? else {
            return Ok(false);
        };
        if library.provider_id != self.provider_id {
            return Err(invalid("library belongs to another provider"));
        }
        Ok(true)
    }
    pub async fn validate_library(&self, txn: &DatabaseTransaction, id: i64) -> Result<(), DbErr> {
        if !self.owns_library(txn, id).await? { return Err(invalid("missing library")); }
        Ok(())
    }
    async fn library(&self, txn: &DatabaseTransaction, key: &ItemKey) -> Result<Option<i64>, DbErr> {
        self.library_optional(txn, key).await?.ok_or_else(|| invalid("missing library"))
    }
    async fn library_optional(&self, txn: &DatabaseTransaction, key: &ItemKey) -> Result<Option<Option<i64>>, DbErr> {
        if key.native_id.is_empty() || key.native_id.len() > 4096 {
            return Err(invalid("invalid native_id"));
        }
        let Patch::Set(library) = key.library_id else {
            return Err(invalid("library_id is required; use null for global items"));
        };
        if let Some(id) = library {
            if !self.owns_library(txn, id).await? { return Ok(None); }
        } else if !matches!(key.r#type, ItemType::Mix | ItemType::Radio) {
            return Err(invalid("only mixes and radio queues may be global"));
        }
        Ok(Some(library))
    }
    async fn allocate(
        &self,
        txn: &DatabaseTransaction,
        item: &ItemRef,
        expected: &[ItemType],
    ) -> Result<m::item::Model, DbErr> {
        match item {
            ItemRef::Id(_) => self.resolve(txn, item, expected).await,
            ItemRef::Key(key) => {
                let library = self.library(txn, key).await?;
                if !expected.contains(&key.r#type) {
                    return Err(invalid("wrong entity type"));
                }
                let now = Timestamp::now();
                let mut query = m::item::Entity::find()
                    .filter(m::item::Column::ProviderId.eq(self.provider_id))
                    .filter(m::item::Column::NativeId.eq(&key.native_id))
                    .filter(m::item::Column::Type.eq(key.r#type));
                query = match library {
                    Some(id) => query.filter(m::item::Column::LibraryId.eq(id)),
                    None => query.filter(m::item::Column::LibraryId.is_null()),
                };
                if let Some(found) = query.one(txn).await? {
                    if key.id.is_some_and(|id| id != found.id) {
                        return Err(invalid("item id does not match key"));
                    }
                    let mut active = found.into_active_model();
                    active.last_sync_at = Set(now);
                    active.update_at = Set(now);
                    return active.update(txn).await;
                }
                if key.id.is_some() {
                    return Err(invalid("supplied item id does not exist"));
                }
                m::item::ActiveModel {
                    id: NotSet,
                    native_id: Set(key.native_id.clone()),
                    provider_id: Set(self.provider_id),
                    library_id: Set(library),
                    r#type: Set(key.r#type),
                    create_at: Set(now),
                    update_at: Set(now),
                    last_sync_at: Set(now),
                }
                .insert(txn)
                .await
            }
        }
    }
    pub async fn sync_batch(&self, batch: SyncBatch) -> Result<SyncResult, DbErr> {
        AlbumInput::validate_mask(&batch.options.albums)?;
        ArtistInput::validate_mask(&batch.options.artists)?;
        SongInput::validate_mask(&batch.options.songs)?;
        RemoteMixInput::validate_mask(&batch.options.remote_mixes)?;
        DynamicInput::validate_mask(&batch.options.dynamics)?;
        let count = batch.libraries.len()
            + batch.albums.len()
            + batch.artists.len()
            + batch.songs.len()
            + batch.remote_mixes.len()
            + batch.dynamics.len()
            + batch.images.len()
            + batch.remote_mix_contents.len();
        if count > 500 {
            return Err(invalid("batch entity limit exceeded"));
        }
        let refs = batch.song_albums.len() * 2
            + batch
                .album_artists
                .iter()
                .chain(&batch.song_artists)
                .map(|r| r.targets.len() + 1)
                .sum::<usize>();
        if refs > 5000
            || batch
                .remote_mix_contents
                .iter()
                .map(|m| m.songs.len())
                .sum::<usize>()
                > 10000
        {
            return Err(invalid("batch reference limit exceeded"));
        }
        struct Budget(usize);
        impl std::io::Write for Budget {
            fn write(&mut self, bytes: &[u8]) -> std::io::Result<usize> {
                self.0 = self.0.saturating_add(bytes.len());
                if self.0 > 4 * 1024 * 1024 {
                    return Err(std::io::Error::other("batch byte limit exceeded"));
                }
                Ok(bytes.len())
            }
            fn flush(&mut self) -> std::io::Result<()> {
                Ok(())
            }
        }
        serde_json::to_writer(Budget(0), &batch).map_err(|e| invalid(e.to_string()))?;
        if let Some(generation) = &self.generation {
            generation.check()?;
        }
        let guard = self
            .coordinator
            .write(self.provider_id, self.run.map(|v| v.0))
            .await?;
        let txn = self.db.begin().await?;
        let result = self.apply(&txn, batch, &guard).await?;
        if let Some(generation) = &self.generation {
            generation.check()?;
        }
        guard.check_origin(self.run.map(|v| v.0))?;
        txn.commit().await?;
        Ok(result)
    }
    async fn apply(
        &self,
        txn: &DatabaseTransaction,
        batch: SyncBatch,
        guard: &super::sync_run::WriteGuard,
    ) -> Result<SyncResult, DbErr> {
        let mut result = SyncResult::default();
        let mut seen = HashSet::new();
        for library in &batch.libraries {
            if library.native_id.is_empty() || !seen.insert(library.native_id.clone()) {
                return Err(invalid("duplicate or empty library identity"));
            }
            let old = m::library::Entity::find()
                .filter(m::library::Column::ProviderId.eq(self.provider_id))
                .filter(m::library::Column::NativeId.eq(&library.native_id))
                .one(txn)
                .await?;
            let model = match old {
                Some(old) => {
                    let mut a = old.into_active_model();
                    a.name = Set(library.name.clone());
                    a.edit_time = Set(chrono::Utc::now());
                    a.update(txn).await?
                }
                None => {
                    m::library::ActiveModel {
                        library_id: NotSet,
                        provider_id: Set(self.provider_id),
                        name: Set(library.name.clone()),
                        native_id: Set(library.native_id.clone()),
                        edit_time: Set(chrono::Utc::now()),
                    }
                    .insert(txn)
                    .await?
                }
            };
            result.libraries.push(model);
        }
        let mut ids = HashSet::new();
        macro_rules! allocate {
            ($group:ident, $types:expr) => {
                for entry in &batch.$group {
                    let item = self.allocate(txn, &entry.item, $types).await?;
                    if !ids.insert(item.id) {
                        return Err(invalid("duplicate entity identity"));
                    }
                    result.items.push(item);
                }
            };
        }
        allocate!(albums, &[ItemType::Album]);
        allocate!(artists, &[ItemType::Artist]);
        allocate!(songs, &[ItemType::Song]);
        allocate!(remote_mixes, &[ItemType::Mix, ItemType::Radio]);
        let mut new_songs = HashSet::new();
        macro_rules! write_entities {
            ($group:ident, $model:ident, $types:expr, $default:expr) => {
                for entry in &batch.$group {
                    let item = self.resolve(txn, &entry.item, $types).await?;
                    let old = m::$model::Entity::find_by_id(item.id).one(txn).await?;
                    let inserted = old.is_none();
                    if inserted && stringify!($group) == "songs" {
                        new_songs.insert(item.id);
                    }
                    let mut model = old.unwrap_or_else(|| ($default)(item.id));
                    entry.apply(&mut model, inserted, &batch.options.$group)?;
                    if model.name.is_empty() {
                        return Err(invalid("entity name is required"));
                    }
                    let active = model.into_active_model().reset_all();
                    if inserted {
                        result.$group.push(active.insert(txn).await?);
                    } else {
                        result.$group.push(active.update(txn).await?);
                    }
                }
            };
        }
        write_entities!(albums, album, &[ItemType::Album], |id| m::album::Model {
            id,
            name: String::new(),
            sort_name: None,
            track_count: 0,
            disc_count: 0,
            r#type: AlbumType::Album,
            duration: 0,
            language: None,
            description: None,
            company: None,
            publish_time: None,
            added_at: None
        });
        write_entities!(artists, artist, &[ItemType::Artist], |id| {
            m::artist::Model {
                id,
                name: String::new(),
                sort_name: None,
                description: String::new(),
                album_count: 0,
                music_count: 0,
                added_at: None,
            }
        });
        write_entities!(songs, song, &[ItemType::Song], |id| m::song::Model {
            id,
            name: String::new(),
            sort_name: None,
            album_id: None,
            track_number: 0,
            disc_number: 0,
            duration: 0,
            can_play: true,
            popularity: 0.0,
            tags: serde_json::json!([]),
            publish_time: None,
            added_at: None
        });
        write_entities!(
            remote_mixes,
            remote_mix,
            &[ItemType::Mix, ItemType::Radio],
            |id| m::remote_mix::Model {
                id,
                name: String::new(),
                description: None,
                track_count: 0,
                mix_type: "normal".into()
            }
        );
        for album in &result.albums {
            if album.duration < 0 || album.track_count < 0 || album.disc_count < 0 {
                return Err(invalid("negative album quantity"));
            }
        }
        for artist in &result.artists {
            if artist.album_count < 0 || artist.music_count < 0 {
                return Err(invalid("negative artist quantity"));
            }
        }
        for song in &result.songs {
            if song.duration < 0
                || song.track_number < 0
                || song.disc_number < 0
                || !song.popularity.is_finite()
            {
                return Err(invalid("invalid song quantity"));
            }
            if !song
                .tags
                .as_array()
                .is_some_and(|tags| tags.iter().all(serde_json::Value::is_string))
            {
                return Err(invalid("song tags must be an array of strings"));
            }
        }
        for remote in &result.remote_mixes {
            if remote.track_count < 0 {
                return Err(invalid("negative mix quantity"));
            }
            let old = m::mix::Entity::find()
                .filter(m::mix::Column::RemoteId.eq(remote.id))
                .one(txn)
                .await?;
            if let Some(old) = old {
                let mut active = old.into_active_model();
                active.name = Set(remote.name.clone());
                active.description = Set(remote.description.clone().unwrap_or_default());
                active.track_count = Set(remote.track_count);
                active.update_at = Set(Timestamp::now());
                active.update(txn).await?;
            } else {
                m::mix::ActiveModel {
                    id: NotSet,
                    name: Set(remote.name.clone()),
                    description: Set(remote.description.clone().unwrap_or_default()),
                    remote_id: Set(Some(remote.id)),
                    track_count: Set(remote.track_count),
                    mix_type: Set(MixType::Cache),
                    create_at: Set(Timestamp::now()),
                    update_at: Set(Timestamp::now()),
                    ..Default::default()
                }
                .insert(txn)
                .await?;
            }
        }
        let mut dynamic_ids = HashSet::new();
        for entry in &batch.dynamics {
            let item = self.resolve(txn, &entry.item, &[]).await?;
            if !dynamic_ids.insert(item.id) {
                return Err(invalid("duplicate dynamic identity"));
            }
            let old = m::dynamic::Entity::find_by_id(item.id).one(txn).await?;
            let inserted = old.is_none();
            let mut model = old.unwrap_or_else(|| m::dynamic::Model {
                id: item.id,
                ..Default::default()
            });
            let favorite = model.favorite_at;
            if let Patch::Set(value) = entry.favorite {
                if !matches!(entry.favorite_at, Patch::Missing) {
                    return Err(invalid("favorite and favorite_at conflict"));
                }
                if inserted || batch.options.dynamics.allows("favorite") {
                    model.favorite_at = if value {
                        model.favorite_at.or_else(|| Some(Timestamp::now()))
                    } else {
                        None
                    };
                }
            }
            entry.apply(&mut model, inserted, &batch.options.dynamics)?;
            if self
                .run
                .is_some_and(|(_, revision)| guard.favorite_changed_after(item.id, revision))
            {
                model.favorite_at = favorite;
            }
            if model.remote_play_count < 0 {
                return Err(invalid("negative remote play count"));
            }
            model.update_at = Timestamp::now();
            if inserted {
                model.into_active_model().reset_all().insert(txn).await?;
            } else {
                m::dynamic::ActiveModel {
                    id: Set(model.id),
                    is_external: Set(model.is_external),
                    remote_play_count: Set(model.remote_play_count),
                    remote_last_played_at: Set(model.remote_last_played_at),
                    favorite_at: Set(model.favorite_at),
                    update_at: Set(model.update_at),
                    ..Default::default()
                }
                .update(txn)
                .await?;
            }
        }
        for item in &result.items {
            m::dynamic::Entity::insert(m::dynamic::ActiveModel {
                id: Set(item.id),
                ..Default::default()
            })
            .on_conflict(
                sea_query::OnConflict::column(m::dynamic::Column::Id)
                    .do_nothing()
                    .to_owned(),
            )
            .do_nothing()
            .exec(txn)
            .await?;
        }
        let mut image_ids = HashSet::new();
        for entry in &batch.images {
            let item = self.resolve(txn, &entry.item_id, &[]).await?;
            if !image_ids.insert((item.id, entry.image_type as i32)) {
                return Err(invalid("duplicate image identity"));
            }
            let old = m::image::Entity::find()
                .filter(m::image::Column::ItemId.eq(item.id))
                .filter(m::image::Column::ImageType.eq(entry.image_type))
                .one(txn)
                .await?;
            if entry.remove {
                if let Some(old) = old {
                    old.delete(txn).await?;
                }
                continue;
            }
            let inserted = old.is_none();
            let mut active = old
                .map(IntoActiveModel::into_active_model)
                .unwrap_or_else(|| m::image::ActiveModel {
                    item_id: Set(item.id),
                    image_type: Set(entry.image_type),
                    native_id: Set(None),
                    fresh: Set(String::new()),
                    db: Set(None),
                    timestamp: Set(chrono::Utc::now()),
                    ..Default::default()
                });
            if let Patch::Set(value) = &entry.native_id {
                active.native_id = Set(value.clone());
            }
            if let Patch::Set(value) = &entry.fresh {
                active.fresh = Set(value.clone());
            }
            active.timestamp = Set(chrono::Utc::now());
            if inserted {
                active.insert(txn).await?;
            } else {
                active.update(txn).await?;
            }
        }
        let mut song_album_ids = HashSet::new();
        for entry in &batch.songs {
            if let Patch::Set(album) = &entry.album_id {
                let song = self.resolve(txn, &entry.item, &[ItemType::Song]).await?;
                if !new_songs.contains(&song.id) && !batch.options.songs.allows("album_id") {
                    continue;
                }
                song_album_ids.insert(song.id);
                let album_id = match album {
                    Some(key) => {
                        let album = self.resolve(txn, key, &[ItemType::Album]).await?;
                        if song.library_id != album.library_id {
                            return Err(invalid("song and album must share a library"));
                        }
                        Some(album.id)
                    }
                    None => None,
                };
                m::song::ActiveModel {
                    id: Set(song.id),
                    album_id: Set(album_id),
                    ..Default::default()
                }
                .update(txn)
                .await?;
            }
        }
        for entry in &batch.song_albums {
            let song = self.resolve(txn, &entry.song, &[ItemType::Song]).await?;
            if !song_album_ids.insert(song.id) {
                return Err(invalid("duplicate song album relation"));
            }
            let album = match &entry.album {
                Some(key) => {
                    let album = self.resolve(txn, key, &[ItemType::Album]).await?;
                    if song.library_id != album.library_id {
                        return Err(invalid("song and album must share a library"));
                    }
                    Some(album.id)
                }
                None => None,
            };
            m::song::ActiveModel {
                id: Set(song.id),
                album_id: Set(album),
                ..Default::default()
            }
            .update(txn)
            .await?;
        }
        macro_rules! relations {
            ($group:ident, $model:ident, $owner:ident, $column:ident, $types:expr) => {
                let mut owners = HashSet::new();
                for entry in &batch.$group {
                    let owner = self.resolve(txn, &entry.owner, $types).await?;
                    if !owners.insert(owner.id) {
                        return Err(invalid("duplicate relation owner"));
                    }
                    let mut targets = HashSet::new();
                    for target in &entry.targets {
                        let artist = self.resolve(txn, target, &[ItemType::Artist]).await?;
                        if artist.library_id != owner.library_id {
                            return Err(invalid("artist relation crosses libraries"));
                        }
                        if !targets.insert(artist.id) {
                            return Err(invalid("duplicate relation target"));
                        }
                    }
                    if matches!(entry.mode, RelationMode::Replace) {
                        m::$model::Entity::delete_many()
                            .filter(m::$model::Column::$column.eq(owner.id))
                            .exec(txn)
                            .await?;
                    }
                    for artist_id in targets {
                        m::$model::Entity::insert(m::$model::ActiveModel {
                            id: NotSet,
                            $owner: Set(owner.id),
                            artist_id: Set(artist_id),
                            update_at: Set(Timestamp::now()),
                        })
                        .on_conflict(
                            sea_query::OnConflict::columns([
                                m::$model::Column::$column,
                                m::$model::Column::ArtistId,
                            ])
                            .update_column(m::$model::Column::UpdateAt)
                            .to_owned(),
                        )
                        .exec(txn)
                        .await?;
                    }
                }
            };
        }
        relations!(
            album_artists,
            rel_album_artist,
            album_id,
            AlbumId,
            &[ItemType::Album]
        );
        relations!(
            song_artists,
            rel_song_artist,
            song_id,
            SongId,
            &[ItemType::Song]
        );
        let mut mixes = HashSet::new();
        for entry in &batch.remote_mix_contents {
            if !matches!(entry.mode, RelationMode::Replace) {
                return Err(invalid("ordered mix contents require replace"));
            }
            let remote = self
                .resolve(txn, &entry.mix, &[ItemType::Mix, ItemType::Radio])
                .await?;
            if !mixes.insert(remote.id) {
                return Err(invalid("duplicate mix contents"));
            }
            let mix = m::mix::Entity::find()
                .filter(m::mix::Column::RemoteId.eq(remote.id))
                .one(txn)
                .await?
                .ok_or_else(|| invalid("missing cache mix"))?;
            let mut unique = HashSet::new();
            let mut songs = Vec::new();
            for key in &entry.songs {
                let song = self.resolve(txn, key, &[ItemType::Song]).await?;
                if !unique.insert(song.id) {
                    return Err(invalid("duplicate_member"));
                }
                songs.push(song.id);
            }
            m::rel_mix_song::Entity::delete_many()
                .filter(m::rel_mix_song::Column::MixId.eq(mix.id))
                .exec(txn)
                .await?;
            for (order, song_id) in songs.iter().enumerate() {
                m::rel_mix_song::ActiveModel {
                    id: NotSet,
                    mix_id: Set(mix.id),
                    song_id: Set(*song_id),
                    order_idx: Set(order as i64),
                    update_at: Set(Timestamp::now()),
                }
                .insert(txn)
                .await?;
            }
            m::remote_mix::ActiveModel {
                id: Set(remote.id),
                track_count: Set(songs.len() as i32),
                ..Default::default()
            }
            .update(txn)
            .await?;
            let now = Timestamp::now();
            m::mix::ActiveModel {
                id: Set(mix.id),
                track_count: Set(songs.len() as i32),
                update_at: Set(now),
                content_update_at: Set(now),
                ..Default::default()
            }
            .update(txn)
            .await?;
        }
        for remote in &mut result.remote_mixes {
            *remote = m::remote_mix::Entity::find_by_id(remote.id)
                .one(txn)
                .await?
                .ok_or_else(|| invalid("missing remote mix"))?;
        }
        // Return merged rows after relation writes, in the caller's input order.
        for song in &mut result.songs {
            *song = m::song::Entity::find_by_id(song.id)
                .one(txn)
                .await?
                .ok_or_else(|| invalid("missing song"))?;
        }
        Ok(result)
    }
}
