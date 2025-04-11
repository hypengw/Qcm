#include "Qcm/status/process.hpp"
#include "core/asio/basic.h"
#include "Qcm/util/ex.hpp"
#include "Qcm/global.hpp"
#include "Qcm/app.hpp"
#include "Qcm/status/provider_status.hpp"

void qcm::process_msg(msg::QcmMessage&& msg) {
    using M = msg::MessageTypeGadget::MessageType;
    switch (msg.type()) {
    case M::PROVIDER_META_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = std::move(msg)] {
            auto p = App::instance()->provider_meta_status();
            p->sync(msg.providerMetaStatusMsg().metas());
        });
        break;
    }
    case M::PROVIDER_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = std::move(msg)] {
            Global::instance()->app_state()->set_state(state::AppState::Session {});
            auto p = App::instance()->provider_status();
            for (auto& s : msg.providerStatusMsg().statuses()) {
                log::info("{}", s.name());
            }
            auto view = std::views::transform(msg.providerStatusMsg().statuses(), [](auto&& el) {
                return qcm::model::ProviderStatus { el };
            });
            p->sync(view);
        });
        break;
    }
    case M::PROVIDER_SYNC_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = std::move(msg)] {
            auto p = App::instance()->provider_status();
            p->updateSyncStatus(msg.providerSyncStatusMsg().status());
        });
        break;
    }
    default: {
    }
    }
}