#pragma once

#include <asio/recycling_allocator.hpp>
#include <asio/bind_executor.hpp>
#include <asio/strand.hpp>

#include "qcm_interface/async.h"
#include "asio_helper/watch_dog.h"
#include "asio_qt/qt_watcher.h"

namespace qcm
{
template<typename Ex, typename Fn>
void QAsyncResult::spawn(Ex&& ex, Fn&& f, const std::source_location loc) {
    helper::QWatcher<QAsyncResult> self { this };
    auto                           main_ex { get_executor() };
    auto                           alloc = asio::recycling_allocator<void>();
    if (use_queue()) {
        push(f, loc);
    } else {
        asio::co_spawn(ex,
                       watch_dog().watch(ex, std::forward<Fn>(f), asio::chrono::minutes(3), alloc),
                       asio::bind_allocator(alloc, [self, main_ex, loc](std::exception_ptr p) {
                           if (! p) return;
                           try {
                               std::rethrow_exception(p);
                           } catch (const std::exception& e) {
                               std::string e_str = e.what();
                               asio::post(main_ex, [self, e_str]() {
                                   if (self) {
                                       self->set_error(QString::fromStdString(e_str));
                                       self->set_status(Status::Error);
                                   }
                               });
                               LogManager::instance()->log(LogLevel::ERROR, loc, "{}", e_str);
                           }
                       }));
    }
}
} // namespace qcm