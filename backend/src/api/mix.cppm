module;
#include <rstd/macro.hpp>
export module qcm.backend:api.mix;
export import :protocol.message;
export import :error;
export import rstd;
export import sqlito;
import :db.mix.write;
import :identity;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
namespace mix_api
{
auto ok_rsp(i32 id) -> p_qcm::p_msg::M_QcmMessage {
    auto rsp       = Box<p_qcm::p_msg::M_Rsp>::make();
    rsp->f_code    = i32();
    rsp->f_message = String::make();
    return qcm_message(id, rstd::move(rsp));
}
} // namespace mix_api

auto create_mix(sqlito::Connection& db, SyncCoordinator& coordinator, i32 request_id,
                const p_qcm::p_msg::M_CreateMixReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace mix_api;
    rstd_try(checked_ids(entry.f_song_ids, "song_ids"_str));
    auto mix_id     = rstd_try(create_mix_record(db, coordinator, entry.f_name.clone()));
    auto rsp        = Box<p_qcm::p_msg::M_CreateMixRsp>::make();
    auto rsp_id     = Box<p_qcm::p_msg::p_id::M_MixId>::make();
    rsp_id->f_value = mix_id;
    rsp->f_id       = Some(rstd::move(rsp_id));
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

auto delete_mix(sqlito::Connection& db, SyncCoordinator& coordinator, i32 request_id,
                const p_qcm::p_msg::M_DeleteMixReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace mix_api;
    auto ids = rstd_try(checked_ids(entry.f_ids, "mix_ids"_str));
    rstd_try(delete_mix_records(db, coordinator, rstd::move(ids)));
    return Ok(ok_rsp(request_id));
}

auto link_mix(sqlito::Connection& db, SyncCoordinator& coordinator, i32 request_id,
              const p_qcm::p_msg::M_LinkMixReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace mix_api;
    auto ids = rstd_try(checked_ids(entry.f_ids, "mix_ids"_str));
    rstd_try(link_mix_records(db, coordinator, rstd::move(ids)));
    return Ok(ok_rsp(request_id));
}

auto manipulate_mix(sqlito::Connection& db, SyncCoordinator& coordinator, i32 request_id,
                    const p_qcm::p_msg::M_MixManipulateReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace mix_api;
    auto mix_id = rstd_try(required_id(entry.f_id, "mix_id"_str));
    auto songs  = rstd_try(checked_ids(entry.f_song_ids, "song_ids or album_ids"_str));
    auto albums = rstd_try(checked_ids(entry.f_album_ids, "song_ids or album_ids"_str));
    auto oper   = entry.f_oper.to_primitive();
    if (oper != 1 && oper != 2 && oper != 3)
        return Err(ProcessError::internal("unsupported mix operation"_str));
    auto change = oper == 1   ? MixContentChange::AppendSongs(rstd::move(songs))
                  : oper == 2 ? MixContentChange::RemoveSongs(rstd::move(songs))
                              : MixContentChange::AppendAlbums(rstd::move(albums));
    auto count  = rstd_try(
        write_mix_content(db, coordinator, MixContentWrite { mix_id, rstd::move(change) }));
    auto rsp        = Box<p_qcm::p_msg::M_MixManipulateRsp>::make();
    auto rsp_id     = Box<p_qcm::p_msg::p_id::M_MixId>::make();
    rsp_id->f_value = mix_id;
    rsp->f_id       = Some(rstd::move(rsp_id));
    rsp->f_oper     = entry.f_oper;
    rsp->f_count    = i64(static_cast<long long>(count.to_primitive()));
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}
} // namespace qcm::backend
