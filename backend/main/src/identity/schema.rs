use prost::Message;
use prost_types::{
    field_descriptor_proto::{Label, Type},
    FileDescriptorSet,
};
use std::collections::BTreeMap;

#[test]
fn domain_fields_use_new_tags_and_reserved_scalar_tags() {
    let schema =
        FileDescriptorSet::decode(include_bytes!(concat!(env!("OUT_DIR"), "/qcm.pb")).as_slice())
            .unwrap();
    let mut messages = BTreeMap::new();
    for file in &schema.file {
        for message in &file.message_type {
            messages.insert(format!("{}.{}", file.package(), message.name()), message);
        }
    }
    let cases: &[(&str, &[(&str, &str, i32)])] = &[
        (
            "SongId",
            &[
                ("model.Song", "id", 1),
                ("entity.SongUpdate", "id", 1),
                ("GetSubtitleReq", "song_id", 1),
                ("PlaylogReq", "song_id", 1),
            ],
        ),
        (
            "SongIds",
            &[
                ("GetSongsRsp", "ids", 5),
                ("GetAlbumRsp", "song_ids", 6),
                ("GetMixSongsRsp", "ids", 7),
                ("GetSongsByIdReq", "ids", 1),
                ("GetSongsByIdRsp", "ids", 5),
                ("GetSongIdsRsp", "ids", 1),
                ("GetQueueNextReq", "current_song_ids", 2),
                ("GetQueueNextRsp", "song_ids", 4),
                ("OpenQueueRsp", "song_ids", 4),
                ("CreateMixReq", "song_ids", 2),
                ("MixManipulateReq", "song_ids", 3),
            ],
        ),
        (
            "AlbumId",
            &[
                ("model.Album", "id", 1),
                ("model.Song", "album_id", 6),
                ("entity.AlbumUpdate", "id", 1),
                ("entity.AlbumLink", "id", 1),
                ("filter.AlbumIdFilter", "value", 1),
                ("GetAlbumReq", "id", 1),
                ("GetAlbumRsp", "id", 5),
            ],
        ),
        (
            "AlbumIds",
            &[
                ("GetAlbumsRsp", "ids", 5),
                ("GetArtistAlbumRsp", "ids", 5),
                ("MixManipulateReq", "album_ids", 4),
            ],
        ),
        (
            "ArtistId",
            &[
                ("model.Artist", "id", 1),
                ("entity.ArtistUpdate", "id", 1),
                ("filter.ArtistIdFilter", "value", 1),
                ("filter.AlbumArtistIdFilter", "value", 1),
                ("GetArtistReq", "id", 1),
                ("GetArtistRsp", "id", 3),
                ("GetArtistAlbumReq", "id", 1),
            ],
        ),
        (
            "ArtistIds",
            &[
                ("entity.SongRelations", "artists", 2),
                ("entity.AlbumRelations", "artists", 1),
                ("GetArtistsRsp", "ids", 5),
                ("GetAlbumArtistsRsp", "ids", 5),
            ],
        ),
        (
            "MixId",
            &[
                ("model.Mix", "id", 1),
                ("entity.MixUpdate", "id", 1),
                ("filter.MixIdFilter", "value", 1),
                ("GetMixSongsReq", "id", 1),
                ("GetMixSongsRsp", "mix_id", 6),
                ("GetMixReq", "id", 1),
                ("GetMixRsp", "id", 3),
                ("CreateMixRsp", "id", 1),
                ("MixManipulateReq", "id", 1),
                ("MixManipulateRsp", "id", 1),
                ("UpdateMixReq", "id", 1),
            ],
        ),
        (
            "MixIds",
            &[
                ("GetMixsRsp", "ids", 5),
                ("GetRemoteMixsRsp", "ids", 5),
                ("DeleteMixReq", "ids", 1),
                ("LinkMixReq", "ids", 1),
            ],
        ),
        (
            "ProviderId",
            &[
                ("model.Library", "provider_id", 3),
                ("model.ProviderStatus", "id", 1),
                ("model.ProviderSyncStatus", "id", 1),
                ("model.ForYouCard", "provider_id", 1),
                ("model.HomeRecommendation", "provider_id", 1),
                ("model.CommentsResult", "provider_id", 3),
                ("CreateTmpProviderReq", "network_provider_id", 2),
                ("UpdateProviderReq", "provider_id", 1),
                ("GetProviderSettingsReq", "provider_id", 1),
                ("ReplaceProviderReq", "provider_id", 1),
                ("DeleteProviderReq", "provider_id", 1),
                ("SyncReq", "provider_id", 1),
                ("GetHomeBlocksReq", "provider_id", 1),
                ("GetHomeBlockItemsReq", "provider_id", 1),
            ],
        ),
        (
            "ProviderIds",
            &[
                ("GetRadioQueuesReq", "provider_ids", 1),
                ("GetHomeRecommendationsReq", "provider_ids", 1),
            ],
        ),
        ("LibraryId", &[("model.Library", "library_id", 1)]),
        (
            "LibraryIds",
            &[
                ("GetAlbumsReq", "library_id", 1),
                ("GetArtistsReq", "library_id", 1),
                ("GetAlbumArtistsReq", "library_id", 1),
                ("GetMixsReq", "library_id", 1),
                ("GetSongIdsReq", "library_ids", 5),
                ("SearchReq", "library_id", 2),
            ],
        ),
        (
            "RadioQueueId",
            &[
                ("model.RadioQueue", "id", 1),
                ("GetQueueNextReq", "queue_id", 1),
                ("OpenQueueReq", "queue_id", 1),
            ],
        ),
        ("RadioId", &[("model.Radio", "id", 1)]),
        ("ProgramId", &[("model.Program", "id", 1)]),
    ];
    let mut checked = 0;
    for (kind, fields) in cases {
        for (owner, name, old) in *fields {
            let key = format!("qcm.msg.{owner}");
            let message = messages[&key];
            let field = message
                .field
                .iter()
                .find(|field| field.name() == *name)
                .unwrap();
            assert_eq!(field.number(), old + 1000, "{key}.{name}");
            assert_eq!(field.type_name(), format!(".qcm.msg.id.{kind}"));
            assert_eq!(field.r#type(), Type::Message);
            assert_eq!(field.label(), Label::Optional);
            assert!(
                message
                    .reserved_range
                    .iter()
                    .any(|range| range.start() <= *old && range.end() > *old),
                "{key}.{name} old tag"
            );
            checked += 1;
        }
    }
    let actual = messages
        .iter()
        .filter(|(name, _)| !name.starts_with("qcm.msg.id."))
        .flat_map(|(_, message)| &message.field)
        .filter(|field| field.type_name().starts_with(".qcm.msg.id."))
        .count();
    assert_eq!(checked, actual, "unlisted domain field");
    for kind in [
        "Song",
        "Album",
        "Artist",
        "Mix",
        "Provider",
        "Library",
        "RadioQueue",
        "Radio",
        "Program",
    ] {
        let message = messages[&format!("qcm.msg.id.{kind}Ids")];
        assert_eq!(message.field.len(), 1);
        assert_eq!(message.field[0].name(), "values");
        assert_eq!(message.field[0].r#type(), Type::Int64);
        assert_eq!(message.field[0].label(), Label::Repeated);
    }
    let exceptions = [
        ("entity.Removal", "id"),
        ("model.ImageReference", "id"),
        ("model.QueuePresentation", "source_item_id"),
        ("SyncItemReq", "id"),
        ("SetFavoriteReq", "id"),
        ("GetShareLinkReq", "id"),
        ("GetCommentsReq", "id"),
        ("SetCommentLikeReq", "id"),
        ("OpenQueueReq", "input_id"),
        ("PlaylogReq", "source_id"),
    ];
    for (owner, message) in &messages {
        if !owner.starts_with("qcm.msg.") || owner.starts_with("qcm.msg.id.") {
            continue;
        }
        for field in &message.field {
            let name = field.name();
            if field.r#type() == Type::Int64
                && (name == "id" || name.ends_with("_id") || name.ends_with("_ids"))
            {
                assert!(
                    exceptions
                        .iter()
                        .any(|(message, field)| owner == &format!("qcm.msg.{message}")
                            && name == *field),
                    "unexpected scalar {owner}.{name}"
                );
            }
        }
    }
}
