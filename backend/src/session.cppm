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
    struct Active {
        u64              id;
        Arc<SessionSlot> slot;
        Sender<Bytes>    events;
        Receiver<Bytes>  incoming;
        Active(u64 id, Arc<SessionSlot> slot, tuple<Sender<Bytes>, Receiver<Bytes>> channel)
            : id(id),
              slot(rstd::move(slot)),
              events(rstd::move(channel.get<0>())),
              incoming(rstd::move(channel.get<1>())) {}
    };
    Option<Arc<Active>> active_;
    u64                 next_id_ { 1 };

    static auto publish(Sender<Bytes> sender, Bytes frame) -> coro<rstd::io::Result<empty>> {
        rstd_co_try(
            co_await sender.send(rstd::move(frame)),
            rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::BrokenPipe }));
        co_return Ok(empty {});
    }
    static auto ignored() -> coro<rstd::io::Result<empty>> { co_return Ok(empty {}); }

public:
    auto can_connect() const -> bool { return active_.is_none(); }
    auto active_session() const -> Option<Arc<SessionSlot>> {
        if (active_.is_none()) return None();
        return Some((*active_)->slot.clone());
    }
    auto add(Arc<SessionSlot> slot) -> Option<u64> {
        if (active_.is_some()) return None();
        auto id = next_id_++;
        active_ = Some(Arc<Active>::make(id, rstd::move(slot), channel<Bytes>(usize(1024))));
        return Some(id);
    }
    void remove(u64 id) {
        if (active_.is_none() || (*active_)->id != id) return;
        (*active_)->slot->close();
        (*active_)->incoming.close();
        active_ = None();
    }
    auto broadcast(Bytes frame) const -> coro<rstd::io::Result<empty>> {
        if (active_.is_none() || ! (*active_)->slot->protocol()->ready()) return ignored();
        return publish((*active_)->events.clone(), rstd::move(frame));
    }
    auto dispatch(u64 id) -> coro<rstd::io::Result<empty>> {
        if (active_.is_none() || (*active_)->id != id) co_return Ok(empty {});
        auto active = (*active_).clone();
        for (;;) {
            auto event = co_await active->incoming.recv();
            if (event.is_none()) co_return Ok(empty {});
            rstd_co_try(co_await active->slot->push(rstd::move(*event)));
        }
    }
    auto len() const -> usize { return active_.is_some() ? usize(1) : usize(); }
};
} // namespace qcm::backend
