module;
#include "core/log.h"

module qcm;
import :status.process;
import :status.app_state;
import :app;
import :global;
import :msg;
import qcm.log;

void qcm::process_msg(msg::QcmMessage&& msg) {
    using M = msg::MessageTypeGadget::MessageType;
    switch (msg.type()) {
    case M::PROVIDER_META_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = rstd::move(msg)] {
            auto p = App::instance()->provider_meta_status();
            p->sync(msg.providerMetaStatusMsg().metas());
        });
        break;
    }
    case M::PROVIDER_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = rstd::move(msg)] {
            auto p = App::instance()->provider_status();
            for (auto& s : msg.providerStatusMsg().statuses()) {
                LOG_INFO("{}", s.name());
            }
            auto view =
                cppstd::views::transform(msg.providerStatusMsg().statuses(), [](auto&& el) {
                    return qcm::model::ProviderStatus { el };
                });
            auto size = view.size();
            p->sync(view);

            auto state = App::instance()->app_state();
            if (! state->is_state<AppState::Error>()) {
                if (size == 0) {
                    state->set_state(AppState::Welcome {});
                } else {
                    state->set_state(AppState::Main {});
                }
            }
        });
        break;
    }
    case M::PROVIDER_SYNC_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = rstd::move(msg)] {
            auto p = App::instance()->provider_status();
            p->updateSyncStatus(msg.providerSyncStatusMsg().status());
        });
        break;
    }
    default: {
    }
    }
}
