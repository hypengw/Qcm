module qcm.backend;
import :event;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;

namespace qcm::backend
{
EventSink::EventSink(Send send): send_(make_rc<Send>(rstd::move(send))) {}

auto EventSink::send_owned(Rc<Send> send, QcmMessage message) -> coro<ProcessResult<empty>> {
    co_return co_await (*send)->operator()(rstd::move(message));
}

auto EventSink::clone() const -> EventSink { return EventSink(*this); }

auto EventSink::send_message(QcmMessage message) const -> coro<ProcessResult<empty>> {
    return send_owned(send_.clone(), rstd::move(message));
}

ResponseSink::ResponseSink(Send send): send_(make_rc<Send>(rstd::move(send))) {}

auto ResponseSink::send_owned(Rc<Send> send, i32 id, ProcessResult<QcmMessage> response)
    -> coro<ProcessResult<empty>> {
    co_return co_await (*send)->operator()(id, rstd::move(response));
}

auto ResponseSink::clone() const -> ResponseSink { return ResponseSink(*this); }

auto ResponseSink::respond(i32 id, ProcessResult<QcmMessage> response) const
    -> coro<ProcessResult<empty>> {
    return send_owned(send_.clone(), id, rstd::move(response));
}
} // namespace qcm::backend
