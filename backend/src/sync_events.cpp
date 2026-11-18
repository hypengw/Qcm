module;
#include <rstd/enum.hpp>
module qcm.backend;
import :sync_events;
import :event;
import :provider.runtime;
import :sync_state;
import :protocol.message;
import rstd.log;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::mpsc::channel;
using rstd::async::mpsc::Receiver;
using rstd::async::mpsc::Sender;
using namespace rstd::prelude;

namespace qcm::backend::sync_event_detail
{
struct Source {
    i64              provider_id;
    ProviderSnapshot snapshot;
    auto             is_current() const -> bool { return snapshot.is_current(); }
    auto             same(const Source& other) const -> bool {
        return provider_id == other.provider_id && snapshot.stamp == other.snapshot.stamp &&
               ProviderHandle::ptr_eq(snapshot.instance, other.snapshot.instance);
    }
};
class Event {
    RSTD_ENUM(
        Event,
        (Commit, (Rc<Source> source; SyncCommit value;)),
        (RefreshProviders, (Rc<Source> source;)),
        (Flush, (rstd::async::oneshot::Sender<empty> completed;))
    )
};

// Publishers and the consumer run on the provider's current-thread runtime.
struct Publication {
    Sender<Event>                    sender;
    rstd::async::watch::Sender<bool> invalidated { false };
    bool                             started {};

    explicit Publication(Sender<Event> sender): sender(rstd::move(sender)) {}
};
auto cancelled(Rc<Publication> publication, Rc<Source> source) -> coro<empty> {
    auto changed = publication->invalidated.subscribe();
    while (! publication->sender.is_closed() && source->is_current())
        (void)co_await changed.changed();
    co_return empty {};
}
} // namespace qcm::backend::sync_event_detail

namespace qcm::backend
{

SyncPublisher::SyncPublisher(const SyncPublisher&)                        = default;
auto SyncPublisher::operator=(const SyncPublisher&) -> SyncPublisher&     = default;
SyncPublisher::SyncPublisher(SyncPublisher&&) noexcept                    = default;
auto SyncPublisher::operator=(SyncPublisher&&) noexcept -> SyncPublisher& = default;
SyncPublisher::~SyncPublisher()                                           = default;

SyncPublisher::SyncPublisher(Rc<sync_event_detail::Publication> publication,
                             Rc<sync_event_detail::Source>      source)
    : publication_(rstd::move(publication)), source_(rstd::move(source)) {}

auto SyncPublisher::published() -> coro<empty> { co_return empty {}; }

auto SyncPublisher::enqueue(sync_event_detail::Event event) const -> bool {
    if (! publication_->started || publication_->sender.is_closed()) return false;
    if (! source_->is_current()) {
        (void)publication_->invalidated.send_replace(true);
        return false;
    }
    return publication_->sender.try_send(rstd::move(event)).is_ok();
}

auto SyncPublisher::send_owned(Rc<sync_event_detail::Publication> publication,
                               Rc<sync_event_detail::Source> source, SyncCommit commit)
    -> coro<bool> {
    if (! publication->started || publication->sender.is_closed()) co_return false;
    if (! source->is_current()) {
        (void)publication->invalidated.send_replace(true);
        co_return false;
    }
    auto sent = co_await rstd::async::select(
        rstd::async::into_future(sync_event_detail::cancelled(publication.clone(), source.clone())),
        rstd::async::into_future(publication->sender.send(
            sync_event_detail::Event::Commit(source.clone(), rstd::move(commit)))));
    co_return sent.is_right() && rstd::move(sent).unwrap_right().is_ok();
}

auto SyncPublisher::clone() const -> SyncPublisher {
    return SyncPublisher(publication_.clone(), source_.clone());
}

auto SyncPublisher::publish(SyncCommit commit) const -> bool {
    return enqueue(sync_event_detail::Event::Commit(source_.clone(), rstd::move(commit)));
}

auto SyncPublisher::send(SyncCommit commit) const -> coro<bool> {
    return send_owned(publication_.clone(), source_.clone(), rstd::move(commit));
}

auto SyncPublisher::refresh_providers() const -> bool {
    return enqueue(sync_event_detail::Event::RefreshProviders(source_.clone()));
}

auto SyncPublisher::progress(SyncProgressKind kind, i32 count) const -> coro<empty> {
    switch (kind) {
    case SyncProgressKind::Album: (void)publish(SyncCommit::AddAlbum(count)); break;
    case SyncProgressKind::Artist: (void)publish(SyncCommit::AddArtist(count)); break;
    case SyncProgressKind::Song: (void)publish(SyncCommit::AddSong(count)); break;
    }
    return published();
}

struct SyncEvents::Status {
    Rc<sync_event_detail::Source> source;
    SyncEventStatus               value;
};

struct SyncEvents::State {
    Rc<sync_event_detail::Publication> publication;
    Receiver<sync_event_detail::Event> receiver;
    Vec<Status>                        statuses;
    Option<Populate>                   populate;
    Option<Refresh>                    refresh;
    Option<EventSink>                  sink;
    rstd::async::watch::Sender<bool>   completed { false };
    State(): State(channel<sync_event_detail::Event>(usize(1024))) {}
    explicit State(
        rstd::tuple<Sender<sync_event_detail::Event>, Receiver<sync_event_detail::Event>> channel)
        : publication(make_rc<sync_event_detail::Publication>(rstd::move(channel.get<0>()))),
          receiver(rstd::move(channel.get<1>())) {}
};

struct SyncEvents::Consumer {
    Option<AbortOnDropHandle<empty>> task;
};

struct SyncEvents::Completion {
    Rc<State> state;
    bool      armed { true };
    explicit Completion(Rc<State> state): state(rstd::move(state)) {}
    Completion(const Completion&) = delete;
    Completion(Completion&& other)
        : state(rstd::move(other.state)), armed(rstd::exchange(other.armed, false)) {}
    ~Completion() {
        if (! armed) return;
        state->receiver.close();
        (void)state->publication->invalidated.send_replace(true);
        (void)state->completed.send_replace(true);
    }
};

SyncEvents::SyncEvents(): state_(make_rc<State>()), consumer_(make_rc<Consumer>()) {}

auto SyncEvents::can_start() const -> bool {
    return ! state_->publication->started && ! state_->publication->sender.is_closed();
}

auto SyncEvents::start_owned(Populate populate, Refresh refresh) -> void {
    state_.get()->populate                   = Some(rstd::move(populate));
    state_.get()->refresh                    = Some(rstd::move(refresh));
    state_.get()->publication.get()->started = true;
    consumer_.get()->task                    = Some(AbortOnDropHandle<empty> {
        rstd::async::spawn_local(run(Completion { state_.clone() }, state_.clone())) });
}

auto SyncEvents::publisher(ProviderRuntime& runtime, i64 provider_id) const
    -> Option<SyncPublisher> {
    return publisher(runtime, provider_id, ProviderHandle {});
}

auto SyncEvents::merge(State& state, const Rc<sync_event_detail::Source>& source,
                       const SyncCommit& commit) -> SyncEventStatus {
    Status* current = nullptr;
    for (auto& status : state.statuses) {
        if (status.value.provider_id != source->provider_id) continue;
        current = &status;
        break;
    }
    if (current == nullptr) {
        state.statuses.push({ source.clone(), { source->provider_id } });
        current = &state.statuses[state.statuses.len() - usize(1)];
    } else if (! current->source->same(*source)) {
        current->source = source.clone();
        current->value  = SyncEventStatus { source->provider_id };
    }
    auto& value = current->value;
    RSTD_MATCH(commit) {
        RSTD_CASE(SetState, state) {
            value = SyncEventStatus { value.provider_id, i32(static_cast<int>(state)) };
        }
        RSTD_CASE(AddAlbum, count) { value.albums = value.albums + count; }
        RSTD_CASE(AddArtist, count) { value.artists = value.artists + count; }
        RSTD_CASE(AddSong, count) { value.songs = value.songs + count; }
    }
    return value;
}

auto SyncEvents::deliver(Rc<State> state, Rc<sync_event_detail::Source> source,
                         SyncEventStatus value, bool refresh) -> coro<empty> {
    if (state->sink.is_none() || ! source->is_current()) co_return empty {};
    if (refresh) {
        auto message = co_await (*state->refresh)->operator()();
        if (message.is_err()) {
            rstd::log::error("sync provider status failed: {}", message.as_ref().unwrap_err());
            co_return empty {};
        }
        if (source->is_current() && ! state->publication->sender.is_closed() &&
            state->sink.is_some())
            (void)co_await state->sink->send_message(rstd::move(*message));
        co_return empty {};
    }
    auto status      = Box<p_qcm::p_msg::p_model::M_ProviderSyncStatus>::make();
    auto id          = Box<p_qcm::p_msg::p_id::M_ProviderId>::make();
    id->f_value      = value.provider_id;
    status->f_id     = Some(rstd::move(id));
    status->f_state  = value.state;
    status->f_album  = value.albums;
    status->f_artist = value.artists;
    status->f_song   = value.songs;
    auto msg         = Box<p_qcm::p_msg::M_ProviderSyncStatusMsg>::make();
    msg->f_status    = Some(rstd::move(status));
    auto populated   = co_await (*state->populate)->operator()(qcm_message(i32(), rstd::move(msg)));
    if (populated.is_err()) {
        rstd::log::error("sync status entities failed: {}", populated.as_ref().unwrap_err());
        co_return empty {};
    }
    if (! source->is_current() || state->publication->sender.is_closed() || state->sink.is_none())
        co_return empty {};
    (void)co_await state->sink->send_message(rstd::move(*populated));
    co_return empty {};
}

auto SyncEvents::notify(Rc<State> state, Rc<sync_event_detail::Source> source,
                        SyncEventStatus value, bool refresh) -> coro<empty> {
    if (! source->is_current() || state->publication->sender.is_closed()) co_return empty {};
    (void)co_await rstd::async::select(
        rstd::async::into_future(
            sync_event_detail::cancelled(state->publication.clone(), source.clone())),
        rstd::async::into_future(deliver(state.clone(), source.clone(), value, refresh)));
    co_return empty {};
}

auto SyncEvents::run(Completion completion, Rc<State> state) -> coro<empty> {
    auto on_exit = rstd::move(completion);
    for (;;) {
        auto event = co_await state->receiver.recv();
        if (event.is_none()) break;
        RSTD_MATCH(*event) {
            RSTD_CASE(Commit, source, commit) {
                if (source->is_current()) {
                    auto value = merge(*state.get(), source, commit);
                    co_await notify(state.clone(), source.clone(), value);
                }
            }
            RSTD_CASE(RefreshProviders, source) {
                co_await notify(state.clone(), source.clone(), {}, true);
            }
            RSTD_CASE(Flush, completed) { (void)completed.send(empty {}); }
        }
    }
    co_return empty {};
}

auto SyncEvents::wait_completed(Rc<State> state) -> coro<empty> {
    auto completed = state->completed.subscribe();
    while (! *completed.borrow())
        if ((co_await completed.changed()).is_err()) break;
    co_return empty {};
}

auto SyncEvents::flush_owned(Rc<State> state) -> coro<empty> {
    if (! state->publication->started) co_return empty {};
    auto completed = rstd::async::oneshot::channel<empty>();
    auto sent      = co_await state->publication->sender.send(
        sync_event_detail::Event::Flush(rstd::move(completed.get<0>())));
    if (sent.is_ok())
        (void)co_await rstd::async::select(rstd::move(completed.get<1>()),
                                           rstd::async::into_future(wait_completed(state.clone())));
    else
        co_await wait_completed(state.clone());
    co_return empty {};
}

auto SyncEvents::wait_stopped(Rc<State> state, Rc<Consumer> consumer) -> coro<empty> {
    co_await wait_completed(state.clone());
    if (auto task = consumer.get()->task.take()) (void)co_await rstd::move(*task);
    co_return empty {};
}

SyncEvents::~SyncEvents() { begin_stop(); }

auto SyncEvents::publisher(ProviderRuntime& runtime, i64 provider_id,
                           const ProviderHandle& expected) const -> Option<SyncPublisher> {
    refresh_sources();
    auto source = runtime.snapshot(provider_id);
    if (source.is_none() || ! source->is_current()) return None();
    if (expected && ! ProviderHandle::ptr_eq(source->instance, expected)) return None();
    return Some(SyncPublisher(state_->publication.clone(),
                              make_rc<sync_event_detail::Source>(
                                  sync_event_detail::Source { provider_id, rstd::move(*source) })));
}

auto SyncEvents::set_event_sink(Option<EventSink> sink) -> void {
    state_.get()->sink = rstd::move(sink);
}

auto SyncEvents::refresh_sources() const -> void {
    (void)state_->publication->invalidated.send_replace(true);
}

auto SyncEvents::snapshot() const -> Vec<SyncEventStatus> {
    Vec<SyncEventStatus> result;
    for (const auto& status : state_->statuses)
        if (status.source->is_current()) result.push(SyncEventStatus(status.value));
    return result;
}

auto SyncEvents::flush() const -> coro<empty> { return flush_owned(state_.clone()); }

auto SyncEvents::begin_stop() -> void {
    if (state_->publication->sender.is_closed()) return;
    state_->receiver.close();
    refresh_sources();
    if (! state_->publication->started) (void)state_->completed.send_replace(true);
}

auto SyncEvents::stop() -> coro<empty> {
    begin_stop();
    return wait_stopped(state_.clone(), consumer_.clone());
}

} // namespace qcm::backend
