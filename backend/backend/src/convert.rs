use std::ops::Deref;

use crate::db::filter::album_filter_to_expr;
use crate::error::ProcessError;
use crate::msg::model as proto;
use crate::msg::{self, QcmMessage};
use chrono::Timelike;
use prost::{EncodeError, Message};
use qcm_core as core;
use qcm_core::db::values::Timestamp;
use qcm_core::model as sqlm;
use qcm_core::provider::AuthResult;
use tokio_tungstenite::tungstenite::Message as WsMessage;

trait ToOpt: Sized {
    fn opt(self) -> Option<Self>;
}

impl ToOpt for String {
    fn opt(self) -> Option<Self> {
        if self.is_empty() {
            None
        } else {
            Some(self)
        }
    }
}

pub trait QcmFrom<T>: Sized {
    fn qcm_from(value: T) -> Self;
}

pub trait QcmInto<T>: Sized {
    fn qcm_into(self) -> T;
}

pub trait QcmTryFrom<T>: Sized {
    type Error;

    // Required method
    fn qcm_try_from(value: T) -> Result<Self, Self::Error>;
}
pub trait QcmTryInto<T>: Sized {
    type Error;

    // Required method
    fn qcm_try_into(self) -> Result<T, Self::Error>;
}

// From implies Into
impl<T, U> QcmInto<U> for T
where
    U: QcmFrom<T>,
{
    #[inline]
    fn qcm_into(self) -> U {
        U::qcm_from(self)
    }
}

impl<T, U> QcmTryInto<U> for T
where
    U: QcmTryFrom<T>,
{
    type Error = U::Error;

    #[inline]
    fn qcm_try_into(self) -> Result<U, Self::Error> {
        U::qcm_try_from(self)
    }
}

impl<T, U> QcmFrom<Option<U>> for Option<T>
where
    T: QcmFrom<U>,
{
    #[inline]
    fn qcm_from(v: Option<U>) -> Option<T> {
        v.map(|x| T::qcm_from(x))
    }
}

impl QcmFrom<String> for Option<String> {
    fn qcm_from(v: String) -> Self {
        match v.is_empty() {
            true => Some(v),
            false => None,
        }
    }
}

impl QcmFrom<Option<String>> for String {
    fn qcm_from(v: Option<String>) -> Self {
        v.unwrap_or_default()
    }
}

impl QcmFrom<prost_types::Timestamp> for sea_orm::entity::prelude::DateTimeUtc {
    fn qcm_from(v: prost_types::Timestamp) -> Self {
        let datetime =
            chrono::DateTime::from_timestamp(v.seconds, v.nanos as u32).unwrap_or_default();
        datetime
    }
}

impl QcmFrom<sea_orm::entity::prelude::DateTimeUtc> for prost_types::Timestamp {
    fn qcm_from(v: sea_orm::entity::prelude::DateTimeUtc) -> Self {
        let seconds = v.timestamp();
        let nanos = v.nanosecond() as i32;

        Self { seconds, nanos }
    }
}
impl QcmFrom<prost_types::Timestamp> for qcm_core::db::values::Timestamp {
    fn qcm_from(v: prost_types::Timestamp) -> Self {
        let datetime: chrono::DateTime<chrono::Utc> = v.qcm_into();
        datetime.into()
    }
}

impl QcmFrom<qcm_core::db::values::Timestamp> for prost_types::Timestamp {
    fn qcm_from(v: qcm_core::db::values::Timestamp) -> Self {
        let dt: sea_orm::entity::prelude::DateTimeUtc = v.into();
        dt.qcm_into()
    }
}

impl QcmFrom<proto::auth_info::Method> for core::provider::AuthMethod {
    fn qcm_from(value: proto::auth_info::Method) -> Self {
        use proto::auth_info::Method;
        match value {
            Method::Username(au) => Self::Username {
                username: au.username,
                pw: au.pw,
            },
            Method::Phone(au) => Self::Phone {
                phone: au.phone,
                pw: au.pw,
            },
            Method::Email(au) => Self::Email {
                email: au.email,
                pw: au.pw,
            },
            Method::Qr(au) => Self::Qr { key: au.key },
        }
    }
}

impl QcmFrom<proto::AuthInfo> for core::provider::AuthInfo {
    fn qcm_from(value: proto::AuthInfo) -> Self {
        Self {
            server_url: value.server_url,
            method: value.method.map(|a| a.qcm_into()),
        }
    }
}

impl QcmFrom<core::provider::AuthMethod> for proto::auth_info::Method {
    fn qcm_from(value: core::provider::AuthMethod) -> Self {
        use core::provider::AuthMethod;
        use proto::auth_info::Method;
        match value {
            AuthMethod::Username { username, pw } => {
                Method::Username(proto::UsernameAuth { username, pw })
            }
            AuthMethod::Phone { phone, pw } => Method::Phone(proto::PhoneAuth { phone, pw }),
            AuthMethod::Email { email, pw } => Method::Email(proto::EmailAuth { email, pw }),
            AuthMethod::Qr { key } => Method::Qr(proto::QrAuth { key }),
        }
    }
}

impl QcmFrom<core::provider::AuthInfo> for proto::AuthInfo {
    fn qcm_from(value: core::provider::AuthInfo) -> Self {
        Self {
            server_url: value.server_url,
            method: value.method.map(|a| a.qcm_into()),
        }
    }
}

impl QcmFrom<proto::Library> for core::model::library::Model {
    fn qcm_from(v: proto::Library) -> Self {
        Self {
            provider_id: v.provider_id,
            native_id: v.native_id,
            library_id: v.library_id,
            name: v.name,
            edit_time: v.edit_time.unwrap_or_default().qcm_into(),
        }
    }
}

impl QcmFrom<core::model::library::Model> for proto::Library {
    fn qcm_from(v: core::model::library::Model) -> Self {
        Self {
            provider_id: v.provider_id,
            native_id: v.native_id,
            library_id: v.library_id,
            name: v.name,
            edit_time: Some(v.edit_time.qcm_into()),
        }
    }
}

impl QcmFrom<bool> for sea_orm::Order {
    fn qcm_from(v: bool) -> Self {
        match v {
            true => sea_orm::Order::Asc,
            false => sea_orm::Order::Desc,
        }
    }
}

impl QcmFrom<proto::Album> for core::model::album::Model {
    fn qcm_from(v: proto::Album) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: match v.sort_name.is_empty() {
                true => Some(v.sort_name),
                false => None,
            },
            duration: v.duration,
            language: v.language.opt(),
            r#type: v
                .r#type
                .try_into()
                .unwrap_or(sqlm::type_enum::AlbumType::Album),
            publish_time: v.publish_time.qcm_into(),
            track_count: v.track_count,
            disc_count: v.disc_count,
            description: v.description.opt(),
            company: v.company.opt(),
            added_at: v.added_at.qcm_into(),
        }
    }
}

impl QcmFrom<core::model::album::Model> for proto::Album {
    fn qcm_from(v: core::model::album::Model) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: v.sort_name.unwrap_or_default(),
            duration: v.duration,
            language: v.language.unwrap_or_default(),
            r#type: v.r#type as i32,
            publish_time: v.publish_time.qcm_into(),
            track_count: v.track_count,
            disc_count: v.disc_count,
            description: v.description.unwrap_or_default(),
            company: v.company.unwrap_or_default(),
            added_at: v.added_at.qcm_into(),
        }
    }
}

impl QcmFrom<proto::Artist> for core::model::artist::Model {
    fn qcm_from(v: proto::Artist) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: v.sort_name.qcm_into(),
            description: v.description,
            album_count: v.album_count,
            music_count: v.music_count,
            added_at: v.added_at.qcm_into(),
        }
    }
}

impl QcmFrom<core::model::artist::Model> for proto::Artist {
    fn qcm_from(v: core::model::artist::Model) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: v.sort_name.qcm_into(),
            description: v.description,
            album_count: v.album_count,
            music_count: v.music_count,
            added_at: v.added_at.qcm_into(),
        }
    }
}

impl QcmFrom<proto::Song> for core::model::song::Model {
    fn qcm_from(v: proto::Song) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: v.sort_name.qcm_into(),
            album_id: Some(v.album_id),
            track_number: v.track_number,
            disc_number: v.disc_number,
            duration: v.duration as i64,
            can_play: v.can_play,
            popularity: v.popularity,
            publish_time: v.publish_time.qcm_into(),
            tags: serde_json::Value::Array(
                v.tags
                    .into_iter()
                    .map(|t| serde_json::Value::String(t))
                    .collect(),
            ),
            added_at: None,
        }
    }
}

impl QcmFrom<core::model::song::Model> for proto::Song {
    fn qcm_from(v: core::model::song::Model) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: v.sort_name.qcm_into(),
            album_id: v.album_id.unwrap_or_default(),
            track_number: v.track_number,
            disc_number: v.disc_number,
            duration: v.duration as f64,
            can_play: v.can_play,
            popularity: v.popularity,
            publish_time: v.publish_time.qcm_into(),
            tags: v
                .tags
                .as_array()
                .map(|arr| {
                    arr.iter()
                        .filter_map(|v| v.as_str())
                        .map(String::from)
                        .collect()
                })
                .unwrap_or_default(),
        }
    }
}

impl QcmFrom<proto::Mix> for core::model::mix::Model {
    fn qcm_from(v: proto::Mix) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: v.sort_name.qcm_into(),
            track_count: v.track_count,
            description: v.description,
            mix_type: v
                .mix_type
                .try_into()
                .unwrap_or(sqlm::type_enum::MixType::Normal),
            remote_id: None,
            create_at: Timestamp::now(),
            update_at: Timestamp::now(),
            content_update_at: Timestamp::new(),
        }
    }
}

impl QcmFrom<core::model::mix::Model> for proto::Mix {
    fn qcm_from(v: core::model::mix::Model) -> Self {
        Self {
            id: v.id,
            name: v.name,
            sort_name: v.sort_name.qcm_into(),
            mix_type: v.mix_type as i32,
            track_count: v.track_count,
            description: v.description,
        }
    }
}

impl QcmFrom<core::provider::ProviderMeta> for proto::ProviderMeta {
    fn qcm_from(v: core::provider::ProviderMeta) -> Self {
        Self {
            type_name: v.type_name,
            svg: v.svg.deref().clone(),
            mutable: v.mutable,
            is_script: v.is_script,
            has_server_url: v.has_server_url,
            auth_types: v.auth_types,
        }
    }
}

impl QcmFrom<msg::model::ArtistSort> for sqlm::artist::Column {
    fn qcm_from(v: msg::model::ArtistSort) -> Self {
        use msg::model::ArtistSort;
        match v {
            ArtistSort::Name => Self::Name,
            ArtistSort::SortName => Self::SortName,
            ArtistSort::MusicCount => Self::MusicCount,
            ArtistSort::AlbumCount => Self::AlbumCount,
        }
    }
}

impl QcmTryFrom<msg::filter::AlbumFilter> for sea_orm::sea_query::SimpleExpr {
    type Error = ProcessError;
    fn qcm_try_from(t: msg::filter::AlbumFilter) -> Result<Self, ProcessError> {
        album_filter_to_expr(&t).ok_or(ProcessError::NotImplemented)
    }
}

impl<F, I> QcmFrom<F> for sea_orm::sea_query::Condition
where
    I: QcmTryInto<sea_orm::sea_query::SimpleExpr>,
    F: Iterator<Item = I>,
{
    fn qcm_from(iter: F) -> Self {
        let mut expr = sea_orm::sea_query::Condition::all();
        for item in iter {
            if let Ok(e) = item.qcm_try_into() {
                expr = expr.add(e);
            }
        }
        expr
    }
}

impl QcmFrom<AuthResult> for msg::model::AuthResult {
    fn qcm_from(v: AuthResult) -> Self {
        match v {
            AuthResult::Ok => Self::Ok,
            AuthResult::Failed { message: _ } => Self::Failed,
            AuthResult::WrongPassword => Self::WrongPassword,
            AuthResult::NoSuchEmail => Self::NoSuchEmail,
            AuthResult::NoSuchPhone => Self::NoSuchPhone,
            AuthResult::NoSuchUsername => Self::NoSuchUsername,
            AuthResult::QrExpired => Self::QrExpired,
            AuthResult::QrWaitScan => Self::QrWaitScan,
            AuthResult::QrWaitComform {
                name: _,
                avatar_url: _,
                message: _,
            } => Self::QrWaitComform,
        }
    }
}

impl QcmFrom<AuthResult> for msg::AuthProviderRsp {
    fn qcm_from(v: AuthResult) -> Self {
        let (code, message, qr_name, qr_avatar_url) = match v {
            AuthResult::Failed { message } => (
                msg::model::AuthResult::Failed,
                message,
                String::new(),
                String::new(),
            ),
            AuthResult::QrWaitComform {
                name,
                avatar_url,
                message,
            } => (
                msg::model::AuthResult::QrWaitComform,
                message,
                name,
                avatar_url,
            ),
            r => (r.qcm_into(), String::new(), String::new(), String::new()),
        };
        Self {
            code: code.into(),
            message,
            qr_name,
            qr_avatar_url,
        }
    }
}

impl QcmFrom<AuthResult> for msg::UpdateProviderRsp {
    fn qcm_from(v: AuthResult) -> Self {
        let (code, message) = match v {
            AuthResult::Failed { message } => (msg::model::AuthResult::Failed, message),
            r => (r.qcm_into(), String::new()),
        };
        Self {
            code: code.into(),
            message,
        }
    }
}

impl QcmFrom<core::subtitle::Subtitle> for msg::model::Subtitle {
    fn qcm_from(v: core::subtitle::Subtitle) -> Self {
        Self {
            items: v
                .items
                .into_iter()
                .map(|item| msg::model::SubtitleItem {
                    start: item.start.unwrap_or(-1),
                    end: item.end.unwrap_or(-1),
                    text: item.text.unwrap_or(String::new()),
                })
                .collect(),
        }
    }
}

impl QcmTryFrom<msg::model::ItemType> for core::model::type_enum::ItemType {
    type Error = ProcessError;
    fn qcm_try_from(t: msg::model::ItemType) -> Result<Self, ProcessError> {
        let t_i32: i32 = t.into();
        Self::try_from(t_i32).map_err(|_| ProcessError::NoSuchItemType(t_i32.to_string()))
    }
}

impl QcmFrom<ProcessError> for msg::Rsp {
    fn qcm_from(v: ProcessError) -> Self {
        Self {
            code: match v {
                ProcessError::Internal(_) => msg::ErrorCode::Internal.into(),
                ProcessError::Encode(_) => msg::ErrorCode::Encode.into(),
                ProcessError::Decode(_) => msg::ErrorCode::Decode.into(),
                ProcessError::UnsupportedMessageType(_) => {
                    msg::ErrorCode::UnsupportedMessageType.into()
                }
                ProcessError::UnknownMessageType(_) => msg::ErrorCode::UnknownMessageType.into(),
                ProcessError::UnexpectedPayload(_) => msg::ErrorCode::UnexpectedPayload.into(),
                ProcessError::MissingFields(_) => msg::ErrorCode::MissingFields.into(),
                ProcessError::NoSuchProviderType(_) => msg::ErrorCode::NoSuchProviderType.into(),
                ProcessError::Db(_) => msg::ErrorCode::Db.into(),
                ProcessError::WrongId(_) => msg::ErrorCode::WrongId.into(),
                ProcessError::NoSuchLibrary(_) => msg::ErrorCode::NoSuchLibrary.into(),
                ProcessError::NoSuchProvider(_) => msg::ErrorCode::NoSuchProvider.into(),
                ProcessError::NoSuchAlbum(_) => msg::ErrorCode::NoSuchAlbum.into(),
                ProcessError::NoSuchSong(_) => msg::ErrorCode::NoSuchSong.into(),
                ProcessError::NoSuchArtist(_) => msg::ErrorCode::NoSuchArtist.into(),
                ProcessError::NoSuchMix(_) => msg::ErrorCode::NoSuchMix.into(),
                ProcessError::NoSuchItemType(_) => msg::ErrorCode::NoSuchItemType.into(),
                ProcessError::NoSuchImageType(_) => msg::ErrorCode::NoSuchImageType.into(),
                ProcessError::NoSuchSearchType(_) => msg::ErrorCode::NoSuchSearchType.into(),
                ProcessError::UnsupportedItemType(_) => msg::ErrorCode::UnsupportedItemType.into(),
                ProcessError::ParseSubtitle(_) => msg::ErrorCode::ParseSubtitle.into(),
                ProcessError::NotFound => msg::ErrorCode::NotFound.into(),
                ProcessError::NotImplemented => msg::ErrorCode::NotImplemented.into(),
                ProcessError::HyperBody(_) => msg::ErrorCode::HyperBody.into(),
                ProcessError::Infallible(_) => {
                    panic!("Got infallible error!")
                }
                ProcessError::None => msg::ErrorCode::Ok.into(),
            },
            message: v.to_string(),
        }
    }
}

impl QcmTryFrom<QcmMessage> for WsMessage {
    type Error = EncodeError;
    fn qcm_try_from(msg: QcmMessage) -> Result<Self, EncodeError> {
        let mut buf = Vec::new();
        msg.encode(&mut buf)?;
        Ok(WsMessage::Binary(buf.into()))
    }
}

macro_rules! impl_from_for_qcm_msg {
    ($msg_type:ident) => {
        impl QcmFrom<msg::$msg_type> for QcmMessage {
            fn qcm_from(v: msg::$msg_type) -> Self {
                Self {
                    id: 0,
                    r#type: msg::MessageType::$msg_type.into(),
                    payload: Some(msg::qcm_message::Payload::$msg_type(v)),
                }
            }
        }
    };
}
impl_from_for_qcm_msg!(ProviderMetaStatusMsg);
impl_from_for_qcm_msg!(ProviderStatusMsg);
impl_from_for_qcm_msg!(ProviderSyncStatusMsg);
impl_from_for_qcm_msg!(CreateTmpProviderRsp);
impl_from_for_qcm_msg!(AuthProviderRsp);
impl_from_for_qcm_msg!(UpdateProviderRsp);
impl_from_for_qcm_msg!(GetProviderMetasRsp);
impl_from_for_qcm_msg!(QrAuthUrlRsp);
impl_from_for_qcm_msg!(TestRsp);
impl_from_for_qcm_msg!(Rsp);

impl_from_for_qcm_msg!(GetAlbumsRsp);
impl_from_for_qcm_msg!(GetAlbumRsp);

impl_from_for_qcm_msg!(GetArtistsRsp);
impl_from_for_qcm_msg!(GetAlbumArtistsRsp);
impl_from_for_qcm_msg!(GetArtistRsp);
impl_from_for_qcm_msg!(GetArtistAlbumRsp);

impl_from_for_qcm_msg!(GetMixsRsp);
impl_from_for_qcm_msg!(GetMixRsp);
impl_from_for_qcm_msg!(GetRemoteMixsRsp);
impl_from_for_qcm_msg!(CreateMixReq);
impl_from_for_qcm_msg!(CreateMixRsp);
impl_from_for_qcm_msg!(DeleteMixReq);
impl_from_for_qcm_msg!(GetMixSongsReq);
impl_from_for_qcm_msg!(GetMixSongsRsp);
impl_from_for_qcm_msg!(MixManipulateReq);
impl_from_for_qcm_msg!(MixManipulateRsp);

impl_from_for_qcm_msg!(SearchRsp);
impl_from_for_qcm_msg!(GetSubtitleRsp);

impl_from_for_qcm_msg!(GetQueueNextRsp);
impl_from_for_qcm_msg!(GetRadioQueuesRsp);

impl_from_for_qcm_msg!(GetStorageInfoRsp);
impl_from_for_qcm_msg!(SyncRsp);
impl_from_for_qcm_msg!(GetSongsByIdRsp);
impl_from_for_qcm_msg!(GetSongIdsRsp);

impl_from_for_qcm_msg!(GetHomeBlocksRsp);
impl_from_for_qcm_msg!(GetHomeBlockItemsRsp);

impl QcmFrom<core::model::type_enum::ItemType> for proto::ItemType {
    fn qcm_from(v: core::model::type_enum::ItemType) -> Self {
        proto::ItemType::try_from(v as i32).unwrap_or(proto::ItemType::Unspecified)
    }
}

impl QcmFrom<qcm_core::provider::HomeBlockStyle> for proto::HomeBlockStyle {
    fn qcm_from(v: qcm_core::provider::HomeBlockStyle) -> Self {
        use qcm_core::provider::HomeBlockStyle as S;
        match v {
            S::Unspecified => proto::HomeBlockStyle::Unspecified,
            S::Grid => proto::HomeBlockStyle::Grid,
            S::Carousel => proto::HomeBlockStyle::Carousel,
            S::List => proto::HomeBlockStyle::List,
        }
    }
}

impl QcmFrom<qcm_core::provider::HomeBlock> for proto::HomeBlock {
    fn qcm_from(v: qcm_core::provider::HomeBlock) -> Self {
        let style: proto::HomeBlockStyle = v.style.qcm_into();
        let item_type: proto::ItemType = v.item_type.qcm_into();
        Self {
            native_id: v.native_id,
            title: v.title,
            subtitle: v.subtitle.unwrap_or_default(),
            description: v.description.unwrap_or_default(),
            style: style.into(),
            item_type: item_type.into(),
        }
    }
}
