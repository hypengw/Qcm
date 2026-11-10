module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:api.home;
import :identity;
export import :error;
export import :provider.runtime;
import :protocol.message;
import :db.sync.run;
import :db.database;
import :api.provider;
import :provider.item;
import :pagination;
import :convert;
export import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto home_blocks_response(i32 request_id, Vec<HomeBlock> blocks) -> p_qcm::p_msg::M_QcmMessage {
    auto rsp = Box<p_qcm::p_msg::M_GetHomeBlocksRsp>::make();
    for (auto& source : blocks) {
        rsp->f_blocks.push(rstd::into<Box<p_qcm::p_msg::p_model::M_HomeBlock>>(rstd::move(source)));
    }
    return qcm_message(request_id, rstd::move(rsp));
}
auto home_block_items_response(i32 request_id, const HomeBlockItems& content, Page page)
    -> p_qcm::p_msg::M_QcmMessage {
    auto rsp  = Box<p_qcm::p_msg::M_GetHomeBlockItemsRsp>::make();
    auto more = page.more(i64(content.total.to_primitive()));
    RSTD_MATCH(content.content) {
        RSTD_CASE(Albums, items) {
            auto albums        = Box<p_qcm::p_msg::M_GetAlbumsRsp>::make();
            albums->f_total    = content.total;
            albums->f_has_more = more;
            auto ids           = Box<p_qcm::p_msg::p_id::M_AlbumIds>::make();
            for (const auto& model : items.as_slice()) ids->f_values.push(i64(model.id));
            albums->f_ids  = Some(rstd::move(ids));
            rsp->o_content = Some(
                p_qcm::p_msg::M_GetHomeBlockItemsRsp::C_o_content::make_albums(rstd::move(albums)));
        }
        RSTD_CASE(Mixes, items) {
            auto mixes        = Box<p_qcm::p_msg::M_GetMixsRsp>::make();
            mixes->f_total    = content.total;
            mixes->f_has_more = more;
            auto ids          = Box<p_qcm::p_msg::p_id::M_MixIds>::make();
            for (const auto& model : items.as_slice()) ids->f_values.push(i64(model.id));
            mixes->f_ids   = Some(rstd::move(ids));
            rsp->o_content = Some(
                p_qcm::p_msg::M_GetHomeBlockItemsRsp::C_o_content::make_mixes(rstd::move(mixes)));
        }
        RSTD_CASE(Artists, items) {
            auto artists        = Box<p_qcm::p_msg::M_GetArtistsRsp>::make();
            artists->f_total    = content.total;
            artists->f_has_more = more;
            auto ids            = Box<p_qcm::p_msg::p_id::M_ArtistIds>::make();
            for (const auto& model : items.as_slice()) ids->f_values.push(i64(model.id));
            artists->f_ids = Some(rstd::move(ids));
            rsp->o_content = Some(p_qcm::p_msg::M_GetHomeBlockItemsRsp::C_o_content::make_artists(
                rstd::move(artists)));
        }
        RSTD_CASE(Songs, items) {
            auto songs        = Box<p_qcm::p_msg::M_GetSongsRsp>::make();
            songs->f_total    = content.total;
            songs->f_has_more = more;
            auto ids          = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
            for (const auto& model : items.as_slice()) ids->f_values.push(i64(model.id));
            songs->f_ids   = Some(rstd::move(ids));
            rsp->o_content = Some(
                p_qcm::p_msg::M_GetHomeBlockItemsRsp::C_o_content::make_songs(rstd::move(songs)));
        }
    }
    return qcm_message(request_id, rstd::move(rsp));
}
auto get_home_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                    SyncCoordinator& coordinator, i32 request_id, i64 provider_id,
                    Option<String> block_id, i32 page_number, i32 page_size,
                    ProviderRequestOwnerHandle owner = {})
    -> rstd::async::coro<ProcessResult<p_qcm::p_msg::M_QcmMessage>> {
    auto snapshot = runtime.snapshot(provider_id);
    if (snapshot.is_none())
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
    auto instance   = snapshot->instance.clone();
    auto context    = snapshot->context(db.clone(), coordinator.clone());
    auto page       = page_of(page_number, page_size);
    auto fixed_page = page_number > i32() ? page_number : i32();
    if (owner) {
        owner.get()->provider_id = provider_id;
        owner.get()->instance    = instance;
    }
    if (block_id.is_some()) {
        auto result = co_await instance.get()->home_block_items_async(
            rstd::move(context), rstd::move(*block_id), fixed_page, page.size);
        if (owner) owner.get()->instance.reset();
        if (! snapshot->is_current()) co_return Err(ProcessError::NotAuth());
        auto content = rstd_co_try(rstd::move(result));
        co_return Ok(home_block_items_response(request_id, content, page));
    }
    auto result = co_await instance.get()->home_blocks_async(rstd::move(context));
    if (owner) owner.get()->instance.reset();
    if (! snapshot->is_current()) co_return Err(ProcessError::NotAuth());
    auto blocks = rstd_co_try(rstd::move(result));
    co_return Ok(home_blocks_response(request_id, rstd::move(blocks)));
}
auto get_home_blocks_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                           SyncCoordinator& coordinator, i32 request_id,
                           const p_qcm::p_msg::M_GetHomeBlocksReq& entry,
                           ProviderRequestOwnerHandle              owner = {})
    -> rstd::async::coro<ProcessResult<p_qcm::p_msg::M_QcmMessage>> {
    auto provider_id = rstd_co_try(required_id(entry.f_provider_id, "provider_id"_str));
    co_return co_await get_home_async(
        runtime, db.clone(), coordinator, request_id, provider_id, None(), i32(), i32(1), owner);
}
auto get_home_block_items_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                                SyncCoordinator& coordinator, i32 request_id,
                                const p_qcm::p_msg::M_GetHomeBlockItemsReq& entry,
                                ProviderRequestOwnerHandle                  owner = {})
    -> rstd::async::coro<ProcessResult<p_qcm::p_msg::M_QcmMessage>> {
    auto provider_id = rstd_co_try(required_id(entry.f_provider_id, "provider_id"_str));
    co_return co_await get_home_async(runtime,
                                      db.clone(),
                                      coordinator,
                                      request_id,
                                      provider_id,
                                      Some(entry.f_block_id.clone()),
                                      entry.f_page,
                                      entry.f_page_size,
                                      owner);
}
} // namespace qcm::backend
