module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:event;
export import :protocol.types;
export import :error;
export import rstd;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;

export namespace qcm::backend
{
using QcmMessage = p_qcm::p_msg::M_QcmMessage;
using QcmPayload = QcmMessage::C_o_payload;

class BackendEvent {
    RSTD_ENUM(BackendEvent, (NewProvider, (i64 id;)), (UpdateProvider, (i64 id;)),
              (DeleteProvider, (i64 id;)), (ReplaceProvider, (i64 id;)))
};

class EventSink {
    using Send = Box<dyn<Fn<coro<ProcessResult<empty>>(QcmMessage)>>>;
    Rc<Send> send_;
    explicit EventSink(Send send): send_(make_rc<Send>(rstd::move(send))) {}
    static auto send_owned(Rc<Send> send, QcmMessage message) -> coro<ProcessResult<empty>> {
        co_return co_await (*send)->operator()(rstd::move(message));
    }

public:
    template<class F>
    static auto make(F send) -> EventSink {
        return EventSink(Send::make(rstd::move(send)));
    }
    auto clone() const -> EventSink { return EventSink(*this); }
    auto send_message(QcmMessage message) const -> coro<ProcessResult<empty>> {
        return send_owned(send_.clone(), rstd::move(message));
    }
};

class ResponseSink {
    using Send = Box<dyn<Fn<coro<ProcessResult<empty>>(i32, ProcessResult<QcmMessage>)>>>;
    Rc<Send> send_;
    explicit ResponseSink(Send send): send_(make_rc<Send>(rstd::move(send))) {}
    static auto send_owned(Rc<Send> send, i32 id, ProcessResult<QcmMessage> response)
        -> coro<ProcessResult<empty>> {
        co_return co_await (*send)->operator()(id, rstd::move(response));
    }

public:
    template<class F>
    static auto make(F send) -> ResponseSink {
        return ResponseSink(Send::make(rstd::move(send)));
    }
    auto clone() const -> ResponseSink { return ResponseSink(*this); }
    auto respond(i32 id, ProcessResult<QcmMessage> response) const -> coro<ProcessResult<empty>> {
        return send_owned(send_.clone(), id, rstd::move(response));
    }
};
} // namespace qcm::backend
