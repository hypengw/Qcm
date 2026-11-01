use crate::{error::ProcessError, msg};

pub trait DomainId {
    fn value(&self) -> i64;
}

pub trait IdField {
    fn required(&self, name: &str) -> Result<i64, ProcessError>;
    fn optional(&self, name: &str) -> Result<Option<i64>, ProcessError>;
}

pub trait IdList {
    fn values(&self) -> &[i64];
    fn checked(&self, name: &str) -> Result<&[i64], ProcessError> {
        let values = self.values();
        if values.iter().any(|id| *id <= 0) {
            return Err(ProcessError::WrongId(name.into()));
        }
        Ok(values)
    }
}

impl<T: DomainId> IdField for Option<T> {
    fn required(&self, name: &str) -> Result<i64, ProcessError> {
        let id = self
            .as_ref()
            .ok_or_else(|| ProcessError::MissingFields(name.into()))?
            .value();
        if id <= 0 {
            return Err(ProcessError::WrongId(name.into()));
        }
        Ok(id)
    }
    fn optional(&self, name: &str) -> Result<Option<i64>, ProcessError> {
        self.as_ref().map(|_| self.required(name)).transpose()
    }
}

macro_rules! domain {
    ($($single:ident, $many:ident);* $(;)?) => { $(
        impl DomainId for msg::id::$single { fn value(&self) -> i64 { self.value } }
        impl From<i64> for msg::id::$single {
            fn from(value: i64) -> Self { Self { value } }
        }
        impl From<Vec<i64>> for msg::id::$many {
            fn from(values: Vec<i64>) -> Self { Self { values } }
        }
        impl IdList for Option<msg::id::$many> {
            fn values(&self) -> &[i64] { self.as_ref().map_or(&[], |ids| ids.values.as_slice()) }
        }
    )* };
}
domain! {
    SongId, SongIds; AlbumId, AlbumIds; ArtistId, ArtistIds; MixId, MixIds;
    ProviderId, ProviderIds; LibraryId, LibraryIds; RadioQueueId, RadioQueueIds;
    RadioId, RadioIds; ProgramId, ProgramIds;
}

#[cfg(test)]
mod schema;

pub fn validate_request(message: &msg::QcmMessage) -> Result<(), ProcessError> {
    use msg::qcm_message::Payload;
    let album_filters = match &message.payload {
        Some(Payload::GetArtistReq(req)) => {
            req.id.required("artist_id")?;
            None
        }
        Some(Payload::GetArtistAlbumReq(req)) => {
            req.id.required("artist_id")?;
            None
        }
        Some(Payload::GetAlbumsReq(req)) => Some(req.filters.as_slice()),
        Some(Payload::GetSongIdsReq(req)) => {
            for filter in &req.filters {
                use msg::filter::song_filter::Payload;
                match &filter.payload {
                    Some(Payload::AlbumIdFilter(value)) => {
                        value.value.required("album_filter")?;
                    }
                    Some(Payload::MixIdFilter(value)) => {
                        value.value.required("mix_filter")?;
                    }
                    _ => (),
                }
            }
            Some(req.album_filters.as_slice())
        }
        _ => None,
    };
    for filter in album_filters.into_iter().flatten() {
        use msg::filter::album_filter::Payload;
        match &filter.payload {
            Some(Payload::ArtistIdFilter(value)) => {
                value.value.required("artist_filter")?;
            }
            Some(Payload::AlbumArtistIdFilter(value)) => {
                value.value.required("album_artist_filter")?;
            }
            _ => (),
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use prost::Message;

    #[test]
    fn domain_id_wire_and_validation() {
        for value in [1, 9007199254740993, i64::MAX] {
            macro_rules! check {
                ($($ty:ident),*) => { $(
                    let id = msg::id::$ty { value };
                    let decoded = msg::id::$ty::decode(id.encode_to_vec().as_slice()).unwrap();
                    assert_eq!(Some(decoded).required("id").unwrap(), value);
                )* };
            }
            check!(
                SongId,
                AlbumId,
                ArtistId,
                MixId,
                ProviderId,
                LibraryId,
                RadioQueueId,
                RadioId,
                ProgramId
            );
        }
        assert!(None::<msg::id::ArtistId>.required("artist").is_err());
        for value in [0, -1] {
            assert!(Some(msg::id::ArtistId { value })
                .required("artist")
                .is_err());
        }
        let ids = Some(msg::id::ArtistIds {
            values: vec![7, 3, 7],
        });
        assert_eq!(ids.checked("artists").unwrap(), &[7, 3, 7]);
        assert!(Some(msg::id::ArtistIds { values: vec![7, 0] })
            .checked("artists")
            .is_err());
        let bytes = msg::id::SongIds {
            values: (1..=20000).collect(),
        }
        .encode_to_vec();
        assert_eq!(bytes.len(), 43494);
        let artist = msg::model::Artist {
            id: Some(9007199254740993.into()),
            name: "Rust".into(),
            ..Default::default()
        };
        if let Ok(path) = std::env::var("QCM_ID_INTEROP") {
            if std::env::var("QCM_ID_INTEROP_MODE").as_deref() == Ok("verify") {
                let bytes = std::fs::read(format!("{path}.qt")).unwrap();
                let decoded = msg::model::Artist::decode(bytes.as_slice()).unwrap();
                assert_eq!(decoded.id, artist.id);
                assert_eq!(decoded.name, "Qt");
            } else {
                std::fs::write(path, artist.encode_to_vec()).unwrap();
            }
        }
    }
}
