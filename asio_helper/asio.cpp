#include "core/core.h"

#ifdef __linux__
#    define ASIO_DECL C_DECL_EXPORT
#endif

#undef ASIO_DISABLE_VISIBILITY

#include <asio/impl/src.hpp>

#include "core/log.h"
#include "asio_helper/detached_log.h"

namespace helper
{

asio_detached_log_t::asio_detached_log_t(const std::source_location loc): loc(loc) {}

void asio_detached_log_t::operator()(std::exception_ptr ptr) {
    if (! ptr) return;
    try {
        std::rethrow_exception(ptr);
    } catch (const std::exception& e) {
        auto level = qcm::LogLevel::ERROR;
        auto what  = std::string_view { e.what() };
        if (what.ends_with("Operation aborted.")) {
            level = qcm::LogLevel::WARN;
        }
        qcm::LogManager::instance()->log(level, loc, "{}", e.what());
    }
}
} // namespace helper