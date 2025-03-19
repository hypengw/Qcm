#include "Qcm/status/process.hpp"
#include "asio_helper/basic.h"
#include "qcm_interface/ex.h"
#include "qcm_interface/global.h"

void qcm::process_msg(msg::QcmMessage&& msg) {
    using M = msg::MessageTypeGadget::MessageType;
    switch (msg.type()) {
    case M::PROVIDER_STATUS_MSG: {
        asio::post(qcm::qexecutor(), [msg = std::move(msg)] {
            Global::instance()->app_state()->set_state(state::AppState::Session {});
        });
        break;
    }
    default: {
    }
    }
}