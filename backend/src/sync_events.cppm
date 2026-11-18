module;
#include <rstd/enum.hpp>
export module qcm.backend:sync_events;
import :event;
export import :sync_state;
using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;

namespace qcm::backend::sync_event_detail
{
struct Publication;
struct Source;
class Event;
} // namespace qcm::backend::sync_event_detail

namespace qcm::backend
{
class Provider;
class ProviderRuntime;
enum class SyncProgressKind;
} // namespace qcm::backend

export namespace qcm::backend
{
class SyncCommit {
    RSTD_ENUM(
        SyncCommit,
        (SetState, (SyncState state;)),
        (AddAlbum, (i32 count;)),
        (AddArtist, (i32 count;)),
        (AddSong, (i32 count;))
    )
};

struct SyncEventStatus {
    i64 provider_id {};
    i32 state {}, albums {}, artists {}, songs {};
};

class SyncEvents;
class SyncPublisher {
    Rc<sync_event_detail::Publication> publication_;
    Rc<sync_event_detail::Source>      source_;
    SyncPublisher(Rc<sync_event_detail::Publication> publication,
                  Rc<sync_event_detail::Source>      source);
    friend class SyncEvents;
    static auto published() -> coro<empty>;
    auto        enqueue(sync_event_detail::Event event) const -> bool;
    static auto send_owned(Rc<sync_event_detail::Publication> publication,
                           Rc<sync_event_detail::Source> source, SyncCommit commit) -> coro<bool>;

public:
    SyncPublisher(const SyncPublisher&);
    auto operator=(const SyncPublisher&) -> SyncPublisher&;
    SyncPublisher(SyncPublisher&&) noexcept;
    auto operator=(SyncPublisher&&) noexcept -> SyncPublisher&;
    ~SyncPublisher();
    auto clone() const -> SyncPublisher;
    auto publish(SyncCommit commit) const -> bool;
    auto send(SyncCommit commit) const -> coro<bool>;
    auto refresh_providers() const -> bool;
    auto progress(SyncProgressKind kind, i32 count) const -> coro<empty>;
};

class SyncEvents {
    using Populate = Box<dyn<Fn<coro<ProcessResult<QcmMessage>>(QcmMessage)>>>;
    using Refresh  = Box<dyn<Fn<coro<ProcessResult<QcmMessage>>()>>>;
    struct Status;
    struct State;
    struct Consumer;
    struct Completion;
    Rc<State>    state_;
    Rc<Consumer> consumer_;
    auto         can_start() const -> bool;
    auto         start_owned(Populate populate, Refresh refresh) -> void;

    static auto merge(State& state, const Rc<sync_event_detail::Source>& source,
                      const SyncCommit& commit) -> SyncEventStatus;
    static auto deliver(Rc<State> state, Rc<sync_event_detail::Source> source,
                        SyncEventStatus value, bool refresh = false) -> coro<empty>;
    static auto notify(Rc<State> state, Rc<sync_event_detail::Source> source, SyncEventStatus value,
                       bool refresh = false) -> coro<empty>;
    static auto run(Completion completion, Rc<State> state) -> coro<empty>;
    static auto wait_completed(Rc<State> state) -> coro<empty>;
    static auto flush_owned(Rc<State> state) -> coro<empty>;
    static auto wait_stopped(Rc<State> state, Rc<Consumer> consumer) -> coro<empty>;

public:
    SyncEvents();
    SyncEvents(const SyncEvents&)                    = delete;
    auto operator=(const SyncEvents&) -> SyncEvents& = delete;
    ~SyncEvents();
    template<class F, class R>
    auto start(F populate, R refresh) -> void {
        if (! can_start()) return;
        auto publish = Populate::make(rstd::move(populate));
        start_owned(rstd::move(publish), Refresh::make(rstd::move(refresh)));
    }
    auto publisher(ProviderRuntime& runtime, i64 provider_id) const -> Option<SyncPublisher>;
    auto publisher(ProviderRuntime& runtime, i64 provider_id, const Rc<Provider>& expected) const
        -> Option<SyncPublisher>;
    auto set_event_sink(Option<EventSink> sink) -> void;
    auto refresh_sources() const -> void;
    auto snapshot() const -> Vec<SyncEventStatus>;
    auto flush() const -> coro<empty>;
    auto begin_stop() -> void;
    auto stop() -> coro<empty>;
};
} // namespace qcm::backend
