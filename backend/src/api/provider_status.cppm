export module qcm.backend:api.provider_status;
export import :event;
export import :model.library;
export import :provider.runtime;
import :protocol.message;
import :protocol.provider;
import :convert;
using rstd::collections::BTreeMap;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct ProviderSyncStatus {
    i64 provider_id {};
    i32 state {}, albums {}, artists {}, songs {};
};

auto provider_status_response(Vec<ProviderStatusSnapshot> snapshots, Vec<Library> libraries,
                              slice<ProviderSyncStatus> sync_statuses) -> QcmMessage {
    auto                        rsp = Box<p_qcm::p_msg::M_ProviderStatusMsg>::make();
    BTreeMap<i64, Vec<Library>> grouped_libraries;
    for (auto& library : libraries) {
        auto provider_id = library.provider_id;
        if (auto group = grouped_libraries.get_mut(provider_id)) {
            (*group)->push(rstd::move(library));
        } else {
            Vec<Library> values;
            values.push(rstd::move(library));
            grouped_libraries.insert(provider_id, rstd::move(values));
        }
    }
    for (auto& entry : snapshots) {
        auto provider_id    = entry.check.provider_id;
        auto status         = Box<p_qcm::p_msg::p_model::M_ProviderStatus>::make();
        auto id             = Box<p_qcm::p_msg::p_id::M_ProviderId>::make();
        id->f_value         = provider_id;
        status->f_id        = Some(rstd::move(id));
        status->f_name      = rstd::move(entry.common.name);
        status->f_type_name = rstd::move(entry.common.type_name);
        auto sync           = Box<p_qcm::p_msg::p_model::M_ProviderSyncStatus>::make();
        auto sync_id        = Box<p_qcm::p_msg::p_id::M_ProviderId>::make();
        sync_id->f_value    = provider_id;
        sync->f_id          = Some(rstd::move(sync_id));
        for (const auto& current : sync_statuses) {
            if (current.provider_id != provider_id) continue;
            sync->f_state  = current.state;
            sync->f_album  = current.albums;
            sync->f_artist = current.artists;
            sync->f_song   = current.songs;
            break;
        }
        status->f_sync_status = Some(rstd::move(sync));
        status->f_auth_info =
            Some(rstd::into<Box<p_qcm::p_msg::p_model::M_AuthInfo>>(rstd::move(entry.common.auth)));
        status->f_epoch = entry.check.epoch;
        if (auto group = grouped_libraries.remove(provider_id))
            for (auto& library : *group)
                status->f_libraries.push(
                    rstd::into<Box<p_qcm::p_msg::p_model::M_Library>>(rstd::move(library)));
        rsp->f_statuses.push(rstd::move(status));
    }
    rsp->f_full = true;
    return qcm_message(i32(), rstd::move(rsp));
}
} // namespace qcm::backend
