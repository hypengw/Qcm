module;
#include <ranges>

#include "core/asio/basic.h"
#include "core/log.h"

export module qcm:status.process;
export import :status.app_state;
export import :msg;

namespace qcm
{
export void process_msg(msg::QcmMessage&&);
}
