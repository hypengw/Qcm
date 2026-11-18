module;
#include <rstd/enum.hpp>
export module qcm.backend:event;
export import :protocol.types;
export import :error;
export import rstd;
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
    explicit EventSink(Send send);
    static auto send_owned(Rc<Send> send, QcmMessage message) -> coro<ProcessResult<empty>>;

public:
    template<class F>
    static auto make(F send) -> EventSink {
        return EventSink(Send::make(rstd::move(send)));
    }
    auto clone() const -> EventSink;
    auto send_message(QcmMessage message) const -> coro<ProcessResult<empty>>;
};

class ResponseSink {
    using Send = Box<dyn<Fn<coro<ProcessResult<empty>>(i32, ProcessResult<QcmMessage>)>>>;
    Rc<Send> send_;
    explicit ResponseSink(Send send);
    static auto send_owned(Rc<Send> send, i32 id, ProcessResult<QcmMessage> response)
        -> coro<ProcessResult<empty>>;

public:
    template<class F>
    static auto make(F send) -> ResponseSink {
        return ResponseSink(Send::make(rstd::move(send)));
    }
    auto clone() const -> ResponseSink;
    auto respond(i32 id, ProcessResult<QcmMessage> response) const -> coro<ProcessResult<empty>>;
};
} // namespace qcm::backend
