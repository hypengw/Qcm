export module qcm.backend:protocol.message;
export import :protocol.types;
import rstd;
using namespace rstd::prelude;

export namespace qcm::backend
{
#define QCM_MESSAGE(PAYLOAD, TYPE, FIELD)                                                          \
    auto qcm_message(i32 request_id, Box<p_qcm::p_msg::M_##PAYLOAD> payload)                       \
        -> p_qcm::p_msg::M_QcmMessage {                                                            \
        p_qcm::p_msg::M_QcmMessage message;                                                        \
        message.f_id               = request_id;                                                   \
        message.f_type             = i32(static_cast<int>(p_qcm::p_msg::E_MessageType::v_##TYPE)); \
        message.f_protocol_version = qcm_protocol_version;                                         \
        message.o_payload =                                                                        \
            Some(p_qcm::p_msg::M_QcmMessage::C_o_payload::make_##FIELD(rstd::move(payload)));      \
        return message;                                                                            \
    }

QCM_MESSAGE(ProviderMetaStatusMsg, PROVIDER_META_STATUS_MSG, provider_meta_status_msg)
QCM_MESSAGE(ProviderStatusMsg, PROVIDER_STATUS_MSG, provider_status_msg)
QCM_MESSAGE(ProviderSyncStatusMsg, PROVIDER_SYNC_STATUS_MSG, provider_sync_status_msg)
QCM_MESSAGE(PlayerStateMsg, PLAYER_STATE_MSG, player_state_msg)
QCM_MESSAGE(PlayerQueueMsg, PLAYER_QUEUE_MSG, player_queue_msg)
QCM_MESSAGE(PlayerUiMsg, PLAYER_UI_MSG, player_ui_msg)
QCM_MESSAGE(CreateTmpProviderRsp, CREATE_TMP_PROVIDER_RSP, create_tmp_provider_rsp)
QCM_MESSAGE(AuthProviderRsp, AUTH_PROVIDER_RSP, auth_provider_rsp)
QCM_MESSAGE(UpdateProviderRsp, UPDATE_PROVIDER_RSP, update_provider_rsp)
QCM_MESSAGE(GetProviderMetasRsp, GET_PROVIDER_METAS_RSP, get_provider_metas_rsp)
QCM_MESSAGE(GetProviderSettingsRsp, GET_PROVIDER_SETTINGS_RSP, get_provider_settings_rsp)
QCM_MESSAGE(GetNetworkSettingsRsp, GET_NETWORK_SETTINGS_RSP, get_network_settings_rsp)
QCM_MESSAGE(QrAuthUrlRsp, QR_AUTH_URL_RSP, qr_auth_url_rsp)
QCM_MESSAGE(TestRsp, TEST_RSP, test_rsp)
QCM_MESSAGE(Rsp, RSP, rsp)
QCM_MESSAGE(GetAlbumsRsp, GET_ALBUMS_RSP, get_albums_rsp)
QCM_MESSAGE(GetAlbumRsp, GET_ALBUM_RSP, get_album_rsp)
QCM_MESSAGE(GetArtistsRsp, GET_ARTISTS_RSP, get_artists_rsp)
QCM_MESSAGE(GetAlbumArtistsRsp, GET_ALBUM_ARTISTS_RSP, get_album_artists_rsp)
QCM_MESSAGE(GetArtistRsp, GET_ARTIST_RSP, get_artist_rsp)
QCM_MESSAGE(GetArtistAlbumRsp, GET_ARTIST_ALBUM_RSP, get_artist_album_rsp)
QCM_MESSAGE(GetMixsRsp, GET_MIXS_RSP, get_mixs_rsp)
QCM_MESSAGE(GetMixRsp, GET_MIX_RSP, get_mix_rsp)
QCM_MESSAGE(GetRemoteMixsRsp, GET_REMOTE_MIXS_RSP, get_remote_mixs_rsp)
QCM_MESSAGE(CreateMixReq, CREATE_MIX_REQ, create_mix_req)
QCM_MESSAGE(CreateMixRsp, CREATE_MIX_RSP, create_mix_rsp)
QCM_MESSAGE(DeleteMixReq, DELETE_MIX_REQ, delete_mix_req)
QCM_MESSAGE(GetMixSongsReq, GET_MIX_SONGS_REQ, get_mix_songs_req)
QCM_MESSAGE(GetMixSongsRsp, GET_MIX_SONGS_RSP, get_mix_songs_rsp)
QCM_MESSAGE(MixManipulateReq, MIX_MANIPULATE_REQ, mix_manipulate_req)
QCM_MESSAGE(MixManipulateRsp, MIX_MANIPULATE_RSP, mix_manipulate_rsp)
QCM_MESSAGE(SearchRsp, SEARCH_RSP, search_rsp)
QCM_MESSAGE(GetSubtitleRsp, GET_SUBTITLE_RSP, get_subtitle_rsp)
QCM_MESSAGE(GetQueueNextRsp, GET_QUEUE_NEXT_RSP, get_queue_next_rsp)
QCM_MESSAGE(OpenQueueRsp, OPEN_QUEUE_RSP, open_queue_rsp)
QCM_MESSAGE(GetRadioQueuesRsp, GET_RADIO_QUEUES_RSP, get_radio_queues_rsp)
QCM_MESSAGE(GetStorageInfoRsp, GET_STORAGE_INFO_RSP, get_storage_info_rsp)
QCM_MESSAGE(SyncRsp, SYNC_RSP, sync_rsp)
QCM_MESSAGE(GetSongsByIdRsp, GET_SONGS_BY_ID_RSP, get_songs_by_id_rsp)
QCM_MESSAGE(GetSongIdsRsp, GET_SONG_IDS_RSP, get_song_ids_rsp)
QCM_MESSAGE(GetShareLinkRsp, GET_SHARE_LINK_RSP, get_share_link_rsp)
QCM_MESSAGE(GetCommentsRsp, GET_COMMENTS_RSP, get_comments_rsp)
QCM_MESSAGE(GetCommentRepliesRsp, GET_COMMENT_REPLIES_RSP, get_comment_replies_rsp)
QCM_MESSAGE(SetCommentLikeRsp, SET_COMMENT_LIKE_RSP, set_comment_like_rsp)
QCM_MESSAGE(GetHomeRecommendationsRsp, GET_HOME_RECOMMENDATIONS_RSP, get_home_recommendations_rsp)
QCM_MESSAGE(GetHomeBlocksRsp, GET_HOME_BLOCKS_RSP, get_home_blocks_rsp)
QCM_MESSAGE(GetHomeBlockItemsRsp, GET_HOME_BLOCK_ITEMS_RSP, get_home_block_items_rsp)

#undef QCM_MESSAGE
} // namespace qcm::backend
