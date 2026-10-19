use crate::{
    error::ProviderError,
    event::Event,
    http,
    model::album::Model as AlbumModel,
    model::artist::Model as ArtistModel,
    model::item::Model as ItemModel,
    model::mix::Model as MixModel,
    model::song::Model as SongModel,
    model::type_enum::{ImageType, ItemType},
    subtitle::Subtitle,
    Result,
};
pub mod playback;
pub mod settings;
pub use playback::{PlaybackKind, PlaybackRequest, PreparedPlayback};
pub mod home;
pub use home::{CardAction, CardCover, CardPresentation, CardStyle, ForYouEntry};
pub mod queue;
pub use queue::{
    QueueBatch, QueueDefinition, QueueInput, QueueNextRequest, QueueOpenReason, QueueOpenRequest,
    QueuePresentation,
};

use reqwest::Response;
use sea_orm::DatabaseConnection;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::mpsc::Sender;

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(tag = "type", content = "message")]
pub enum AuthResult {
    Ok,
    Failed {
        message: String,
    },
    WrongPassword,
    NoSuchUsername,
    NoSuchEmail,
    NoSuchPhone,
    QrExpired,
    QrWaitScan,
    QrWaitComform {
        name: String,
        avatar_url: String,
        message: String,
    },
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum AuthMethod {
    Username { username: String, pw: String },
    Phone { phone: String, pw: String },
    Email { email: String, pw: String },
    Qr { key: String },
}

#[derive(Clone, Debug)]
pub struct Context {
    pub db: DatabaseConnection,
    pub cache_db: DatabaseConnection,
    pub ev_sender: Sender<Event>,
    pub sync_coordinator: Arc<crate::db::sync_run::SyncCoordinator>,
}

#[derive(Default, Clone, Debug, Serialize, Deserialize)]
pub struct AuthInfo {
    pub server_url: String,
    pub method: Option<AuthMethod>,
}

#[derive(Default, Clone, Debug, Serialize, Deserialize)]
pub struct QrInfo {
    pub key: String,
    pub url: String,
}

#[derive(Clone, Copy, Debug, Default, Serialize, Deserialize)]
pub enum HomeBlockStyle {
    #[default]
    Unspecified,
    Grid,
    Carousel,
    List,
}

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct HomeBlock {
    pub native_id: String,
    pub title: String,
    pub subtitle: Option<String>,
    pub description: Option<String>,
    pub style: HomeBlockStyle,
    pub item_type: ItemType,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub enum HomeBlockContent {
    Albums(Vec<AlbumModel>),
    Mixes(Vec<MixModel>),
    Artists(Vec<ArtistModel>),
    Songs(Vec<SongModel>),
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct QueueConfigOption {
    pub value: String,
    pub name: String,
    #[serde(default)]
    pub description: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct QueueConfigField {
    pub key: String,
    pub name: String,
    #[serde(default)]
    pub description: String,
    pub value: String,
    pub options: Vec<QueueConfigOption>,
}

/// Creator for provider
///
/// # Parameters
/// - `name`: provider name
/// - `device_id`: device id
///
pub type Creator = dyn Fn(Option<i64>, &str, &str) -> Result<Arc<dyn Provider>> + Send + Sync;

#[derive(Clone)]
pub struct ProviderMeta {
    pub type_name: String,
    pub svg: Arc<String>,
    pub creator: Arc<Creator>,
    pub mutable: bool,
    pub is_script: bool,
    pub has_server_url: bool,
    pub auth_types: Vec<i32>,
}

impl ProviderMeta {
    pub fn new(type_name: &str, auth_types: &[i32], svg: Arc<String>, f: Arc<Creator>) -> Self {
        ProviderMeta {
            type_name: type_name.to_string(),
            svg,
            creator: f,
            mutable: false,
            is_script: false,
            has_server_url: true,
            auth_types: auth_types.to_vec(),
        }
    }
}

pub trait ProviderSession {
    fn load_cookie(&self, data: &str);
    fn save_cookie(&self) -> String;
}

pub trait ProviderCommon {
    fn id(&self) -> Option<i64>;
    fn set_id(&self, id: Option<i64>);
    fn name(&self) -> String;
    fn set_name(&self, name: &str);
    fn type_name(&self) -> String;
    fn base_url(&self) -> String;
    fn auth_method(&self) -> Option<AuthMethod>;
    fn load_auth_info(&self, base_url: &str, auth_method: Option<AuthMethod>);
    fn settings(&self) -> settings::SettingsSnapshot;
    fn set_settings(&self, settings: settings::SettingsSnapshot);
}

#[async_trait::async_trait]
pub trait Provider: ProviderCommon + ProviderSession + Send + Sync {
    fn invalidate(&self) {}
    /// Changes when the account or authentication context is replaced.
    fn lifecycle_epoch(&self) -> u64 {
        0
    }
    async fn persist_session(&self) -> Result<(), ProviderError> {
        Ok(())
    }
    fn session_key(&self) -> Option<String> {
        None
    }
    fn bind_session(&self, _db: DatabaseConnection, _key: String) {}
    fn resource_scope(&self) -> Result<Option<String>, ProviderError> {
        Ok(None)
    }
    fn load(&self, data: &str);
    fn save(&self) -> String;

    fn settings_definition(&self) -> Result<Vec<settings::SettingField>, ProviderError> {
        Ok(Vec::new())
    }
    fn validate_settings(&self, values: &settings::SettingValues) -> Result<settings::SettingValues, settings::SettingsError> {
        let fields = self.settings_definition().map_err(|e| settings::SettingsError::message(e.to_string()))?;
        settings::validate(&fields, values)
    }

    async fn prepare_resource(
        &self,
        _resource: crate::resource::Resource,
    ) -> Result<crate::resource::PreparedResource, ProviderError> {
        Err(ProviderError::NotImplemented)
    }

    async fn open_playback(&self, _request: PlaybackRequest) -> Result<PreparedPlayback, ProviderError> {
        Ok(PreparedPlayback::byte_resource())
    }
    async fn prepare_playback_resource(&self, _state: &str, _uri: &str) -> Result<crate::resource::PreparedResource, ProviderError> {
        Err(ProviderError::NotImplemented)
    }
    async fn keep_playback(&self, _state: &str) -> Result<(), ProviderError> { Ok(()) }
    async fn close_playback(&self, _state: &str) -> Result<(), ProviderError> { Ok(()) }

    async fn check(&self, ctx: &Context) -> Result<(), ProviderError>;
    async fn auth(&self, ctx: &Context, info: &AuthInfo) -> Result<AuthResult, ProviderError>;
    async fn sync(&self, ctx: &Context) -> Result<(), ProviderError>;
    async fn sync_item(&self, ctx: &Context, item: ItemModel) -> Result<(), ProviderError>;
    async fn get_queue_next(
        &self,
        ctx: &Context,
        native_id: &str,
        current_native_ids: &[String],
    ) -> Result<Vec<SongModel>, ProviderError> {
        let _ = (ctx, native_id, current_native_ids);
        Err(ProviderError::NotImplemented)
    }
    async fn queue_configuration(
        &self,
        _ctx: &Context,
        _native_id: &str,
    ) -> Result<Vec<QueueConfigField>, ProviderError> {
        Ok(Vec::new())
    }
    async fn get_queue_next_configured(
        &self,
        ctx: &Context,
        native_id: &str,
        current_native_ids: &[String],
        configuration: &HashMap<String, String>,
    ) -> Result<Vec<SongModel>, ProviderError> {
        if !configuration.is_empty() {
            return Err(ProviderError::NotImplemented);
        }
        self.get_queue_next(ctx, native_id, current_native_ids).await
    }
    async fn queue_definition(
        &self,
        ctx: &Context,
        native_id: &str,
    ) -> Result<QueueDefinition, ProviderError> {
        Ok(QueueDefinition {
            configuration: self.queue_configuration(ctx, native_id).await?,
            ..Default::default()
        })
    }
    async fn open_queue(
        &self,
        ctx: &Context,
        native_id: &str,
        request: QueueOpenRequest,
    ) -> Result<QueueBatch, ProviderError> {
        queue::legacy_open(self, ctx, native_id, request).await
    }
    async fn next_queue(
        &self,
        ctx: &Context,
        native_id: &str,
        request: QueueNextRequest,
    ) -> Result<QueueBatch, ProviderError> {
        queue::legacy_next(self, ctx, native_id, request).await
    }
    async fn close_queue(
        &self,
        _ctx: &Context,
        _native_id: &str,
        _state: &str,
    ) -> Result<(), ProviderError> {
        Ok(())
    }
    async fn favorite(
        &self,
        ctx: &Context,
        item_id: &str,
        item_type: ItemType,
        value: bool,
    ) -> Result<(), ProviderError>;

    async fn qr(&self, _ctx: &Context) -> Result<QrInfo, ProviderError> {
        Err(ProviderError::NotImplemented)
    }

    async fn image(
        &self,
        ctx: &Context,
        item_id: &str,
        image_id: Option<&str>,
        image_type: ImageType,
    ) -> Result<Response, ProviderError>;

    async fn audio(
        &self,
        ctx: &Context,
        item_id: &str,
        headers: Option<http::HeaderMap>,
    ) -> Result<Response, ProviderError>;

    async fn subtitle(&self, item_id: &str) -> Result<Subtitle, ProviderError>;

    async fn for_you_items(&self, _ctx: &Context) -> Result<Vec<ForYouEntry>, ProviderError> {
        Err(ProviderError::NotImplemented)
    }

    async fn home_blocks(&self, ctx: &Context) -> Result<Vec<HomeBlock>, ProviderError> {
        let _ = ctx;
        Err(ProviderError::NotImplemented)
    }

    async fn home_block_items(
        &self,
        ctx: &Context,
        block_id: &str,
        page: i32,
        page_size: i32,
    ) -> Result<(HomeBlockContent, i32), ProviderError> {
        let _ = (ctx, block_id, page, page_size);
        Err(ProviderError::NotImplemented)
    }
}

struct ProviderCommonDataInner {
    id: Option<i64>,
    name: String,
    base_url: String,
    auth_method: Option<AuthMethod>,
    settings: settings::SettingsSnapshot,
}

pub struct ProviderCommonData {
    pub device_id: String,
    meta_type: String,
    inner: std::sync::RwLock<ProviderCommonDataInner>,
}

impl ProviderCommonData {
    pub fn new(id: Option<i64>, name: &str, device_id: &str, meta_type: &str) -> Self {
        Self {
            device_id: device_id.to_string(),
            meta_type: meta_type.to_string(),
            inner: std::sync::RwLock::new(ProviderCommonDataInner {
                id: id,
                name: name.to_string(),
                base_url: String::new(),
                auth_method: None,
                settings: Default::default(),
            }),
        }
    }
    pub fn id(&self) -> Option<i64> {
        self.inner.read().unwrap().id
    }
    pub fn base_url(&self) -> String {
        self.inner.read().unwrap().base_url.clone()
    }
    pub fn auth_method(&self) -> Option<AuthMethod> {
        self.inner.read().unwrap().auth_method.clone()
    }
    pub fn settings(&self) -> settings::SettingsSnapshot {
        self.inner.read().unwrap().settings.clone()
    }
    fn set_id(&self, id: Option<i64>) {
        let mut inner = self.inner.write().unwrap();
        inner.id = id;
    }
}

pub trait HasCommonData {
    fn common<'a>(&'a self) -> &'a ProviderCommonData;
}

impl<T: HasCommonData> ProviderCommon for T {
    fn id(&self) -> Option<i64> {
        self.common().id()
    }
    fn set_id(&self, id: Option<i64>) {
        self.common().set_id(id);
    }
    fn name(&self) -> String {
        self.common().inner.read().unwrap().name.clone()
    }
    fn set_name(&self, name: &str) {
        let mut inner = self.common().inner.write().unwrap();
        inner.name = name.to_string();
    }
    fn type_name(&self) -> String {
        self.common().meta_type.clone()
    }
    fn base_url(&self) -> String {
        self.common().inner.read().unwrap().base_url.clone()
    }
    fn auth_method(&self) -> Option<AuthMethod> {
        self.common().auth_method()
    }
    fn load_auth_info(&self, base_url: &str, auth_method: Option<AuthMethod>) {
        let mut inner = self.common().inner.write().unwrap();
        inner.base_url = base_url.to_string();
        inner.auth_method = auth_method;
    }
    fn settings(&self) -> settings::SettingsSnapshot {
        self.common().settings()
    }
    fn set_settings(&self, settings: settings::SettingsSnapshot) {
        self.common().inner.write().unwrap().settings = settings;
    }
}
