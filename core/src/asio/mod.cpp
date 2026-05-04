module;
#include <cerrno>
module qcm.asio;

import :detached_log;
import qcm.log;

namespace qcm
{
void handle_asio_exception(std::exception_ptr                                   eptr,
                           asio::any_completion_handler<void(std::string_view)> on_error,
                           const std::source_location                           loc) {
    try {
        if (eptr) {
            std::rethrow_exception(eptr);
        }
    } catch (const asio::system_error& ex) {
        auto        level    = qcm::LogLevel::ERROR;
        auto        code     = ex.code().value();
        const auto& category = ex.code().category();
        if (category == asio::error::get_system_category()) {
            if (code == ECANCELED) {
                level = qcm::LogLevel::WARN;
            }
        }

        qcm::log::log(level, loc, "[{}] {}", category.name(), ex.what());
        if (on_error) on_error(ex.what());
    } catch (const std::exception& ex) {
        qcm::log::log(qcm::LogLevel::ERROR, loc, "{}", ex.what());
        if (on_error) on_error(ex.what());
    }
}

asio_detached_log_t::asio_detached_log_t(const std::source_location loc): loc(loc) {}

void asio_detached_log_t::operator()(std::exception_ptr ptr) {
    handle_asio_exception(ptr, {}, loc);
}
} // namespace qcm
