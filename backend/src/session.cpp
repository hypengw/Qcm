module;
#include <rstd/macro.hpp>
module qcm.backend;
import :session;
import :protocol.message;
import liproto;
using rstd::tuple;
using rstd::async::coro;
using rstd::async::mpsc::channel;
using rstd::async::mpsc::Receiver;
using rstd::async::mpsc::Sender;
using rstd::bytes::Bytes;
using rstd::sync::Arc;
using namespace rstd::prelude;

namespace qcm::backend
{
SessionSlot::SessionSlot(tuple<Sender<SessionFrame>, Receiver<SessionFrame>> channel)
    : sender_(rstd::move(channel.get<0>())), receiver_(rstd::move(channel.get<1>())) {}

auto SessionSlot::push_owned(Sender<SessionFrame> sender, SessionFrame frame)
    -> coro<rstd::io::Result<empty>> {
    rstd_co_try(
        co_await sender.send(rstd::move(frame)),
        rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::BrokenPipe }));
    co_return Ok(empty {});
}

SessionSlot::SessionSlot(usize capacity): SessionSlot(channel<SessionFrame>(capacity)) {}

auto SessionSlot::make() -> Option<Arc<SessionSlot>> {
    return Some(Arc<SessionSlot>::make(usize(256)));
}

auto SessionSlot::push(Bytes frame) const -> coro<rstd::io::Result<empty>> {
    return send(SessionFrame { liwebsocketo::Kind::Binary, rstd::move(frame) });
}

auto SessionSlot::send(SessionFrame frame) const -> coro<rstd::io::Result<empty>> {
    return push_owned(sender_.clone(), rstd::move(frame));
}

auto SessionSlot::recv() const -> coro<Option<SessionFrame>> { return receiver_.recv(); }

void SessionSlot::close() {
    protocol_->close();
    receiver_.close();
}

SessionRegistry::Active::Active(u64 id, Arc<SessionSlot> slot,
                                tuple<Sender<Bytes>, Receiver<Bytes>> channel)
    : id(id),
      slot(rstd::move(slot)),
      events(rstd::move(channel.get<0>())),
      incoming(rstd::move(channel.get<1>())) {}

auto SessionRegistry::publish(Sender<Bytes> sender, Bytes frame) -> coro<rstd::io::Result<empty>> {
    rstd_co_try(
        co_await sender.send(rstd::move(frame)),
        rstd::io::Error::from_kind(rstd::io::ErrorKind { rstd::io::ErrorKind::BrokenPipe }));
    co_return Ok(empty {});
}

auto SessionRegistry::ignored() -> coro<rstd::io::Result<empty>> { co_return Ok(empty {}); }

auto SessionRegistry::add(Arc<SessionSlot> slot) -> Option<u64> {
    if (active_.is_some()) return None();
    auto id = next_id_++;
    active_ = Some(Arc<Active>::make(id, rstd::move(slot), channel<Bytes>(usize(1024))));
    return Some(id);
}

void SessionRegistry::remove(u64 id) {
    if (active_.is_none() || (*active_)->id != id) return;
    (*active_)->slot->close();
    (*active_)->incoming.close();
    active_     = None();
    activation_ = None();
}

auto SessionRegistry::activate_ui(String token) -> coro<rstd::io::Result<empty>> {
    if (! token.is_empty() || activation_.is_none()) activation_ = Some(rstd::move(token));
    co_return co_await flush_ui_activation();
}

auto SessionRegistry::flush_ui_activation() -> coro<rstd::io::Result<empty>> {
    if (activation_.is_none() || active_.is_none() || ! (*active_)->slot->protocol()->ready())
        co_return Ok(empty {});
    auto request                = Box<p_qcm::p_msg::M_PlayerUiMsg>::make();
    request->f_raise            = true;
    request->f_activation_token = rstd::move(activation_.take()).unwrap();
    auto encoded = rstd_co_try(liproto::encode_to_vec(qcm_message(i32(), rstd::move(request))),
                               [](auto error) {
                                   return rstd::io::Error::with_source(
                                       rstd::io::ErrorKind { rstd::io::ErrorKind::InvalidData },
                                       Box<dyn<rstd::error::Error>>::make(rstd::move(error)));
                               });
    co_return co_await (*active_)->slot->push(Bytes::copy_from_slice(encoded.as_slice()));
}

auto SessionRegistry::broadcast(Bytes frame) const -> coro<rstd::io::Result<empty>> {
    if (active_.is_none() || ! (*active_)->slot->protocol()->ready()) return ignored();
    return publish((*active_)->events.clone(), rstd::move(frame));
}

auto SessionRegistry::dispatch(u64 id) -> coro<rstd::io::Result<empty>> {
    if (active_.is_none() || (*active_)->id != id) co_return Ok(empty {});
    auto active = (*active_).clone();
    for (;;) {
        auto event = co_await active->incoming.recv();
        if (event.is_none()) co_return Ok(empty {});
        rstd_co_try(co_await active->slot->push(rstd::move(*event)));
    }
}
} // namespace qcm::backend
