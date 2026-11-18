export module qcm.backend:session;
export import rstd;
export import liwebsocketo;
export import :protocol.session;
using rstd::tuple;
using rstd::async::coro;
using rstd::async::mpsc::Receiver;
using rstd::async::mpsc::Sender;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
using SessionFrame = liwebsocketo::Message;

class SessionSlot {
    Sender<SessionFrame>   sender_;
    Receiver<SessionFrame> receiver_;
    Arc<ProtocolSession>   protocol_ { Arc<ProtocolSession>::make() };

    explicit SessionSlot(tuple<Sender<SessionFrame>, Receiver<SessionFrame>> channel);
    static auto push_owned(Sender<SessionFrame> sender, SessionFrame frame)
        -> coro<rstd::io::Result<empty>>;

public:
    explicit SessionSlot(usize capacity);
    static auto make() -> Option<Arc<SessionSlot>>;
    auto push(Bytes frame) const -> coro<rstd::io::Result<empty>>;
    auto send(SessionFrame frame) const -> coro<rstd::io::Result<empty>>;
    auto recv() const -> coro<Option<SessionFrame>>;
    auto protocol() const -> Arc<ProtocolSession> { return protocol_.clone(); }
    void close();
};

class SessionRegistry {
    struct Active {
        u64              id;
        Arc<SessionSlot> slot;
        Sender<Bytes>    events;
        Receiver<Bytes>  incoming;
        Active(u64 id, Arc<SessionSlot> slot, tuple<Sender<Bytes>, Receiver<Bytes>> channel);
    };
    Option<Arc<Active>> active_;
    u64                 next_id_ { 1 };
    Option<String>      activation_;

    static auto publish(Sender<Bytes> sender, Bytes frame) -> coro<rstd::io::Result<empty>>;
    static auto ignored() -> coro<rstd::io::Result<empty>>;

public:
    auto can_connect() const -> bool { return active_.is_none(); }
    auto active_session() const -> Option<Arc<SessionSlot>> {
        if (active_.is_none()) return None();
        return Some((*active_)->slot.clone());
    }
    auto add(Arc<SessionSlot> slot) -> Option<u64>;
    void remove(u64 id);
    auto activate_ui(String token) -> coro<rstd::io::Result<empty>>;
    auto flush_ui_activation() -> coro<rstd::io::Result<empty>>;
    auto broadcast(Bytes frame) const -> coro<rstd::io::Result<empty>>;
    auto dispatch(u64 id) -> coro<rstd::io::Result<empty>>;
    auto len() const -> usize { return active_.is_some() ? usize(1) : usize(); }
};
} // namespace qcm::backend
