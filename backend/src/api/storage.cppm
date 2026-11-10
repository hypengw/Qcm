module;
#include <rstd/macro.hpp>
export module qcm.backend:api.storage;
export import :protocol.message;
export import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;
using QcmMessage = p_qcm::p_msg::M_QcmMessage;

export namespace qcm::backend
{
auto storage_info_response(i64 database_size, i64 media_bytes, i64 image_bytes, i32 request_id)
    -> QcmMessage {
    auto rsp             = Box<p_qcm::p_msg::M_GetStorageInfoRsp>::make();
    rsp->f_media_size    = media_bytes;
    rsp->f_image_size    = image_bytes;
    rsp->f_database_size = database_size;
    return qcm_message(request_id, rstd::move(rsp));
}
} // namespace qcm::backend
