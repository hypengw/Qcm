module;
#include <rstd/enum.hpp>
module qcm.backend;
import :error_response;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using ErrorCode       = p_qcm::p_msg::E_ErrorCode;
using QcmErrorMessage = p_qcm::p_msg::M_QcmMessage;

namespace rstd
{
auto Impl<convert::From<qcm::backend::ProcessError>, Box<p_qcm::p_msg::M_Rsp>>::from(
    qcm::backend::ProcessError error) -> Box<p_qcm::p_msg::M_Rsp> {
    auto result       = Box<p_qcm::p_msg::M_Rsp>::make();
    result->f_message = rstd::format("{}", error);
    auto code         = [&]() -> ErrorCode {
        RSTD_MATCH(error) {
            RSTD_CASE(NotAuth) { return ErrorCode::v_ERROR_CODE_NOT_AUTH; }
            RSTD_CASE(AuthRequired, message, jump_url) { return ErrorCode::v_ERROR_CODE_NOT_AUTH; }
            RSTD_CASE(Radio, cause) {
                RSTD_MATCH(cause) {
                    RSTD_CASE(Expired) { return ErrorCode::v_ERROR_CODE_QUEUE_SESSION_EXPIRED; }
                    RSTD_CASE(Conflict) { return ErrorCode::v_ERROR_CODE_QUEUE_SESSION_CONFLICT; }
                    RSTD_CASE_DEFAULT { return ErrorCode::v_ERROR_CODE_INTERNAL; }
                }
            }
            RSTD_CASE(ProtocolVersion) { return ErrorCode::v_ERROR_CODE_PROTOCOL_VERSION; }
            RSTD_CASE(ProtocolNotReady) { return ErrorCode::v_ERROR_CODE_PROTOCOL_NOT_READY; }
            RSTD_CASE(Encode) { return ErrorCode::v_ERROR_CODE_ENCODE; }
            RSTD_CASE(Decode) { return ErrorCode::v_ERROR_CODE_DECODE; }
            RSTD_CASE(UnsupportedMessageType) {
                return ErrorCode::v_ERROR_CODE_UNSUPPORTED_MESSAGE_TYPE;
            }
            RSTD_CASE(UnknownMessageType) { return ErrorCode::v_ERROR_CODE_UNKNOWN_MESSAGE_TYPE; }
            RSTD_CASE(UnexpectedPayload) { return ErrorCode::v_ERROR_CODE_UNEXPECTED_PAYLOAD; }
            RSTD_CASE(MissingFields) { return ErrorCode::v_ERROR_CODE_MISSING_FIELDS; }
            RSTD_CASE(NoSuchProviderType) { return ErrorCode::v_ERROR_CODE_NO_SUCH_PROVIDER_TYPE; }
            RSTD_CASE(Db) { return ErrorCode::v_ERROR_CODE_DB; }
            RSTD_CASE(HttpBody) { return ErrorCode::v_ERROR_CODE_HYPER_BODY; }
            RSTD_CASE(WrongId) { return ErrorCode::v_ERROR_CODE_WRONG_ID; }
            RSTD_CASE(NoSuchLibrary) { return ErrorCode::v_ERROR_CODE_NO_SUCH_LIBRARY; }
            RSTD_CASE(NoSuchProvider) { return ErrorCode::v_ERROR_CODE_NO_SUCH_PROVIDER; }
            RSTD_CASE(NoSuchAlbum) { return ErrorCode::v_ERROR_CODE_NO_SUCH_ALBUM; }
            RSTD_CASE(NoSuchSong) { return ErrorCode::v_ERROR_CODE_NO_SUCH_SONG; }
            RSTD_CASE(NoSuchArtist) { return ErrorCode::v_ERROR_CODE_NO_SUCH_ARTIST; }
            RSTD_CASE(NoSuchMix) { return ErrorCode::v_ERROR_CODE_NO_SUCH_MIX; }
            RSTD_CASE(NoSuchItemType) { return ErrorCode::v_ERROR_CODE_NO_SUCH_ITEM_TYPE; }
            RSTD_CASE(NoSuchImageType) { return ErrorCode::v_ERROR_CODE_NO_SUCH_IMAGE_TYPE; }
            RSTD_CASE(NoSuchSearchType) { return ErrorCode::v_ERROR_CODE_NO_SUCH_SEARCH_TYPE; }
            RSTD_CASE(UnsupportedItemType) { return ErrorCode::v_ERROR_CODE_UNSUPPORTED_ITEM_TYPE; }
            RSTD_CASE(ParseSubtitle) { return ErrorCode::v_ERROR_CODE_PARSE_SUBTITLE; }
            RSTD_CASE(NotFound) { return ErrorCode::v_ERROR_CODE_NOT_FOUND; }
            RSTD_CASE(NotImplemented) { return ErrorCode::v_ERROR_CODE_NOT_IMPLEMENTED; }
            RSTD_CASE_DEFAULT { return ErrorCode::v_ERROR_CODE_INTERNAL; }
        }
    }();
    result->f_code = i32(static_cast<int>(code));
    if (error.is_AuthRequired()) {
        auto& auth              = error.as_AuthRequired();
        auto  challenge         = Box<p_qcm::p_msg::M_AuthRequired>::make();
        challenge->f_message    = rstd::move(auth.message);
        challenge->f_jump_url   = rstd::move(auth.jump_url);
        result->f_auth_required = Some(rstd::move(challenge));
    }
    return result;
}
} // namespace rstd

namespace qcm::backend
{
auto error_response(i32 id, ProcessError error) -> QcmErrorMessage {
    return qcm_message(id, rstd::into<Box<p_qcm::p_msg::M_Rsp>>(rstd::move(error)));
}

auto ok_response(i32 id) -> QcmErrorMessage {
    auto response    = Box<p_qcm::p_msg::M_Rsp>::make();
    response->f_code = i32(static_cast<int>(ErrorCode::v_ERROR_CODE_OK));
    return qcm_message(id, rstd::move(response));
}

auto http_error_response(const ProcessError& error) -> liserveto::Response {
    rstd::log::warn("HTTP request failed: {}", error);
    auto                status = u16(500);
    liserveto::Response response;
    RSTD_MATCH(error) {
        RSTD_CASE(NotAuth) { status = u16(401); }
        RSTD_CASE(AuthRequired, message, jump_url) { status = u16(401); }
        RSTD_CASE(NotFound) { status = u16(404); }
        RSTD_CASE(NoSuchProvider) { status = u16(404); }
        RSTD_CASE(NoSuchAlbum) { status = u16(404); }
        RSTD_CASE(NoSuchArtist) { status = u16(404); }
        RSTD_CASE(NoSuchSong) { status = u16(404); }
        RSTD_CASE(NoSuchMix) { status = u16(404); }
        RSTD_CASE(NoSuchItemType) { status = u16(404); }
        RSTD_CASE(Resource, cause) {
            status = u16(502);
            RSTD_MATCH(cause) {
                RSTD_CASE(Upstream, upstream_status, headers) {
                    status = u16(upstream_status.to_primitive());
                    for (const auto& header : headers)
                        if (! header.name.matches("content-length"_str) &&
                            ! header.name.matches("transfer-encoding"_str) &&
                            ! header.name.matches("content-encoding"_str))
                            response.head.headers.push(header.clone());
                }
                RSTD_CASE_DEFAULT { break; }
            }
        }
        RSTD_CASE_DEFAULT { break; }
    }
    if (response.head.headers.is_empty())
        (void)response.head.headers.add("cache-control"_str, "no-store"_str);
    auto parsed          = lihttpto::StatusCode::make(status);
    response.head.status = parsed.is_ok() ? *parsed : lihttpto::StatusCode::make(u16(500)).unwrap();
    return response;
}
} // namespace qcm::backend
