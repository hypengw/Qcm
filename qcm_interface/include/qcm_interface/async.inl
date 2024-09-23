#pragma once

#include <asio/recycling_allocator.hpp>

#include "qcm_interface/async.h"
#include "asio_helper/watch_dog.h"

namespace qcm
{
template<typename Ex, typename Fn>
void QAsyncResult::spawn(Ex&& ex, Fn&& f) {
    QPointer<QAsyncResult> self { this };
    auto                   main_ex { get_executor() };
    auto                   alloc = asio::recycling_allocator<void>();
    asio::co_spawn(ex,
                   watch_dog().watch(ex, std::forward<Fn>(f), alloc),
                   asio::bind_allocator(alloc, [self, main_ex](std::exception_ptr p) {
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
                           ERROR_LOG("{}", e_str);
                       }
                   }));
}
} // namespace qcm