module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:api.request_input;
export import :protocol.message;
export import :identity;
export import :event;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class RequestKind
{
    Direct,
    ProviderRequest,
    ProviderChange,
    ItemSync
};
struct RequestRoute {
    QcmPayload::Tag             payload;
    p_qcm::p_msg::E_MessageType type;
    RequestKind                 kind;
};
using MessageType                              = p_qcm::p_msg::E_MessageType;
inline constexpr RequestRoute request_routes[] = {
    { QcmPayload::Tag::test_req, MessageType::v_TEST_REQ, RequestKind::Direct },
    { QcmPayload::Tag::player_control_req, MessageType::v_PLAYER_CONTROL_REQ, RequestKind::Direct },
    { QcmPayload::Tag::player_queue_req, MessageType::v_PLAYER_QUEUE_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_provider_metas_req,
      MessageType::v_GET_PROVIDER_METAS_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::qr_auth_url_req,
      MessageType::v_QR_AUTH_URL_REQ,
      RequestKind::ProviderChange },
    { QcmPayload::Tag::auth_provider_req,
      MessageType::v_AUTH_PROVIDER_REQ,
      RequestKind::ProviderChange },
    { QcmPayload::Tag::add_provider_req,
      MessageType::v_ADD_PROVIDER_REQ,
      RequestKind::ProviderChange },
    { QcmPayload::Tag::update_provider_req,
      MessageType::v_UPDATE_PROVIDER_REQ,
      RequestKind::ProviderChange },
    { QcmPayload::Tag::replace_provider_req,
      MessageType::v_REPLACE_PROVIDER_REQ,
      RequestKind::ProviderChange },
    { QcmPayload::Tag::delete_provider_req,
      MessageType::v_DELETE_PROVIDER_REQ,
      RequestKind::ProviderChange },
    { QcmPayload::Tag::create_tmp_provider_req,
      MessageType::v_CREATE_TMP_PROVIDER_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::delete_tmp_provider_req,
      MessageType::v_DELETE_TMP_PROVIDER_REQ,
      RequestKind::ProviderChange },
    { QcmPayload::Tag::get_provider_settings_req,
      MessageType::v_GET_PROVIDER_SETTINGS_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::get_network_settings_req,
      MessageType::v_GET_NETWORK_SETTINGS_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::update_network_settings_req,
      MessageType::v_UPDATE_NETWORK_SETTINGS_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::get_mixs_req, MessageType::v_GET_MIXS_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_mix_req, MessageType::v_GET_MIX_REQ, RequestKind::Direct },
    { QcmPayload::Tag::create_mix_req, MessageType::v_CREATE_MIX_REQ, RequestKind::Direct },
    { QcmPayload::Tag::delete_mix_req, MessageType::v_DELETE_MIX_REQ, RequestKind::Direct },
    { QcmPayload::Tag::mix_manipulate_req, MessageType::v_MIX_MANIPULATE_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_mix_songs_req,
      MessageType::v_GET_MIX_SONGS_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::link_mix_req, MessageType::v_LINK_MIX_REQ, RequestKind::Direct },
    { QcmPayload::Tag::sync_req, MessageType::v_SYNC_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_albums_req, MessageType::v_GET_ALBUMS_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_album_req, MessageType::v_GET_ALBUM_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_artists_req, MessageType::v_GET_ARTISTS_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_album_artists_req,
      MessageType::v_GET_ALBUM_ARTISTS_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::get_artist_req, MessageType::v_GET_ARTIST_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_artist_album_req,
      MessageType::v_GET_ARTIST_ALBUM_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::get_subtitle_req,
      MessageType::v_GET_SUBTITLE_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_remote_mixs_req,
      MessageType::v_GET_REMOTE_MIXS_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::search_req, MessageType::v_SEARCH_REQ, RequestKind::Direct },
    { QcmPayload::Tag::set_favorite_req,
      MessageType::v_SET_FAVORITE_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_storage_info_req,
      MessageType::v_GET_STORAGE_INFO_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::playlog_req, MessageType::v_PLAYLOG_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_songs_by_id_req,
      MessageType::v_GET_SONGS_BY_ID_REQ,
      RequestKind::Direct },
    { QcmPayload::Tag::get_song_ids_req,
      MessageType::v_GET_SONG_IDS_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_queue_next_req,
      MessageType::v_GET_QUEUE_NEXT_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_radio_queues_req,
      MessageType::v_GET_RADIO_QUEUES_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::open_queue_req,
      MessageType::v_OPEN_QUEUE_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_radio_preview_req,
      MessageType::v_GET_RADIO_PREVIEW_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::close_queue_req,
      MessageType::v_CLOSE_QUEUE_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_share_link_req, MessageType::v_GET_SHARE_LINK_REQ, RequestKind::Direct },
    { QcmPayload::Tag::get_comments_req,
      MessageType::v_GET_COMMENTS_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_comment_replies_req,
      MessageType::v_GET_COMMENT_REPLIES_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::set_comment_like_req,
      MessageType::v_SET_COMMENT_LIKE_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::sync_item_req, MessageType::v_SYNC_ITEM_REQ, RequestKind::ItemSync },
    { QcmPayload::Tag::get_home_blocks_req,
      MessageType::v_GET_HOME_BLOCKS_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_home_block_items_req,
      MessageType::v_GET_HOME_BLOCK_ITEMS_REQ,
      RequestKind::ProviderRequest },
    { QcmPayload::Tag::get_home_recommendations_req,
      MessageType::v_GET_HOME_RECOMMENDATIONS_REQ,
      RequestKind::ProviderRequest },
};
} // namespace qcm::backend

export namespace qcm::backend
{
struct ProviderAction {
    QcmPayload::Tag tag;
    Option<i64>     provider_id;
    String          temporary;

    auto persisted() const -> bool {
        return tag == QcmPayload::Tag::update_provider_req ||
               tag == QcmPayload::Tag::replace_provider_req ||
               tag == QcmPayload::Tag::delete_provider_req;
    }
};

class RequestInput {
    Arc<QcmMessage>        message_;
    RequestKind            kind_;
    Option<ProviderAction> action_;

    RequestInput(Arc<QcmMessage> message, RequestKind kind, Option<ProviderAction> action)
        : message_(rstd::move(message)), kind_(kind), action_(rstd::move(action)) {}
    static auto provider_action(const QcmMessage& request) -> ProviderAction {
        auto tag         = request.o_payload->tag();
        auto provider_id = [](const auto& value) -> Option<i64> {
            return value.is_some() ? Some(i64((*value)->f_value)) : None<i64>();
        };
        RSTD_MATCH(*request.o_payload) {
            RSTD_CASE_PAYLOAD(update_provider_req, entry) {
                return { tag, provider_id(entry->f_provider_id), entry->f_tmp_provider.clone() };
            }
            RSTD_CASE_PAYLOAD(replace_provider_req, entry) {
                return { tag, provider_id(entry->f_provider_id), entry->f_tmp_provider.clone() };
            }
            RSTD_CASE_PAYLOAD(delete_provider_req, entry) {
                return { tag, provider_id(entry->f_provider_id), {} };
            }
            RSTD_CASE_PAYLOAD(add_provider_req, entry) {
                return { tag, None(), entry->f_tmp_provider.clone() };
            }
            RSTD_CASE_PAYLOAD(auth_provider_req, entry) {
                return { tag, None(), entry->f_tmp_provider.clone() };
            }
            RSTD_CASE_PAYLOAD(qr_auth_url_req, entry) {
                return { tag, None(), entry->f_tmp_provider.clone() };
            }
            RSTD_CASE_PAYLOAD(delete_tmp_provider_req, entry) {
                return { tag, None(), entry->f_key.clone() };
            }
            RSTD_CASE_DEFAULT { rstd::panic { "request is not a Provider action" }; }
        }
    }

public:
    static auto prepare(QcmMessage message) -> ProcessResult<RequestInput> {
        if (message.o_payload.is_some()) {
            for (const auto& route : request_routes) {
                if (route.payload != message.o_payload->tag() ||
                    message.f_type != i32(static_cast<int>(route.type)))
                    continue;
                rstd_try(validate_request_identity(message));
                auto action = route.kind == RequestKind::ProviderChange
                                  ? Some(provider_action(message))
                                  : None<ProviderAction>();
                return Ok(RequestInput(
                    Arc<QcmMessage>::make(rstd::move(message)), route.kind, rstd::move(action)));
            }
        }
        return Err(ProcessError::UnsupportedMessageType(message.f_type));
    }
    auto kind() const -> RequestKind { return kind_; }
    auto message() const -> Arc<QcmMessage> { return message_.clone(); }
    auto into_action() && -> ProviderAction { return rstd::move(action_).unwrap(); }
};
} // namespace qcm::backend
