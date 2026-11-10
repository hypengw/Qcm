module;
#include <rstd/macro.hpp>
export module qcm.backend:session;
export import rstd;
export import liwebsocketo;
export import :protocol.session;
using rstd::tuple;
using rstd::async::coro;
using rstd::async::mpsc::channel;
using rstd::async::mpsc::Receiver;
using rstd::async::mpsc::Sender;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
using SessionFrame = liwebsocketo::Message;

class SessionSlot {
    Sender<SessionFrame>   sender_;
    Receiver<SessionFrame> receiver_;
    Arc<ProtocolSession>   protocol_ { Arc<ProtocolSession>::make() };

    explicit SessionSlot(tuple<Sender<SessionFrame>, Receiver<SessionFrame>> channel)
        : sender_(rstd::move(channel.get<0>())), receiver_(rstd::move(channel.get<1>())) {}
    static auto push_owned(Sender<SessionFrame> sender, SessionFrame frame)
        -> coro<rstd::io::Result<empty>> {
        rstd_co_try(
            co_await sender.send(rstd::move(frame)),
            rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::BrokenPipe }));
        co_return Ok(empty {});
    }

public:
    explicit SessionSlot(usize capacity): SessionSlot(channel<SessionFrame>(capacity)) {}
    static auto make() -> Option<Arc<SessionSlot>> {
        return Some(Arc<SessionSlot>::make(usize(256)));
    }
    auto push(Bytes frame) const -> coro<rstd::io::Result<empty>> {
        return send(SessionFrame { liwebsocketo::Kind::Binary, rstd::move(frame) });
    }
    auto send(SessionFrame frame) const -> coro<rstd::io::Result<empty>> {
        return push_owned(sender_.clone(), rstd::move(frame));
    }
    auto recv() const -> coro<Option<SessionFrame>> { return receiver_.recv(); }
    auto protocol() const -> Arc<ProtocolSession> { return protocol_.clone(); }
    void close() {
        protocol_->close();
        receiver_.close();
    }
};

class SessionRegistry {
    struct Event {
        u64   session;
        Bytes frame;
    };
    Sender<Event>                        events_;
    Receiver<Event>                      incoming_;
    Option<tuple<u64, Arc<SessionSlot>>> active_;
    u64                                  next_id_ { 1 };
    bool                                 used_ {};

    explicit SessionRegistry(tuple<Sender<Event>, Receiver<Event>> channel)
        : events_(rstd::move(channel.get<0>())), incoming_(rstd::move(channel.get<1>())) {}
    static auto publish(Sender<Event> sender, Event event) -> coro<rstd::io::Result<empty>> {
        rstd_co_try(
            co_await sender.send(rstd::move(event)),
            rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::BrokenPipe }));
        co_return Ok(empty {});
    }
    static auto ignored() -> coro<rstd::io::Result<empty>> { co_return Ok(empty {}); }

public:
    SessionRegistry(): SessionRegistry(channel<Event>(usize(1024))) {}
    auto can_connect() const -> bool { return ! used_; }
    auto active_session() const -> Option<Arc<SessionSlot>> {
        if (active_.is_none()) return None();
        return Some(active_->get<1>().clone());
    }
    auto add(Arc<SessionSlot> slot) -> Option<u64> {
        if (used_) return None();
        used_   = true;
        auto id = next_id_++;
        active_ = Some(tuple<u64, Arc<SessionSlot>> { id, rstd::move(slot) });
        return Some(id);
    }
    void remove(u64 id) {
        if (active_.is_none() || active_->get<0>() != id) return;
        active_->get<1>()->close();
        active_ = None();
        incoming_.close();
    }
    auto broadcast(Bytes frame) const -> coro<rstd::io::Result<empty>> {
        if (active_.is_none() || ! active_->get<1>()->protocol()->ready()) return ignored();
        return publish(events_.clone(), Event { active_->get<0>(), rstd::move(frame) });
    }
    auto dispatch(u64 id) -> coro<rstd::io::Result<empty>> {
        for (;;) {
            auto event = co_await incoming_.recv();
            if (event.is_none()) co_return Ok(empty {});
            if (event->session != id || active_.is_none() || active_->get<0>() != id) continue;
            auto slot = active_->get<1>().clone();
            rstd_co_try(co_await slot->push(rstd::move(event->frame)));
        }
    }
    auto len() const -> usize { return active_.is_some() ? usize(1) : usize(); }
};
} // namespace qcm::backend
