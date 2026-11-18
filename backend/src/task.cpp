module;
#include <rstd/macro.hpp>
module qcm.backend;
import :task;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::collections::HashMap;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto TaskOwner::Response::ignored() -> coro<ProcessResult<empty>> { co_return Ok(empty {}); }

auto TaskOwner::Response::sink_for(Rc<Response> response) -> ResponseSink {
    return ResponseSink::make(
        [response = rstd::move(response)](i32, ProcessResult<QcmMessage> result) {
            if (response->claimed) return ignored();
            auto state           = response.clone();
            state.get()->claimed = true;
            return response->sink.respond(response->request_id, rstd::move(result));
        });
}

struct TaskOwner::Completion {
    alloc::rc::Weak<TaskOwner> owner;
    Kind                       kind;
    i64                        handle;
    CompletionSender           state;
    bool                       armed { true };

    Completion(alloc::rc::Weak<TaskOwner> owner, Kind kind, i64 handle, CompletionSender state)
        : owner(rstd::move(owner)), kind(kind), handle(handle), state(rstd::move(state)) {}
    Completion(const Completion&) = delete;
    Completion(Completion&& other)
        : owner(rstd::move(other.owner)),
          kind(other.kind),
          handle(other.handle),
          state(rstd::move(other.state)),
          armed(rstd::exchange(other.armed, false)) {}
    ~Completion() {
        if (! armed) return;
        if (auto live = owner.upgrade()) live->get()->finish(kind, handle);
        (void)state.send_replace(true);
    }
};

auto TaskOwner::next(bool stopping) -> ProcessResult<i64> {
    if (stopping) return Err(ProcessError::internal("task owner is stopping"_str));
    if (next_handle_ == i64::MAX.to_primitive())
        return Err(ProcessError::internal("task handles exhausted"_str));
    return Ok(i64(next_handle_++));
}

auto TaskOwner::run(Completion completion, coro<empty> body) -> coro<empty> {
    auto on_exit = rstd::move(completion);
    co_await rstd::move(body);
    co_return empty {};
}

auto TaskOwner::spawn(Kind kind, i64 handle, coro<empty> body, Rc<Response> response) -> Task {
    auto channel    = rstd::async::watch::channel(false);
    auto completion = rstd::move(channel.get<0>());
    completions_.insert(handle, completion.clone());
    auto task = rstd::async::spawn_local(
        run(Completion(self_.clone(), kind, handle, completion.clone()), rstd::move(body)));
    return Task { handle, AbortOnDropHandle<empty> { rstd::move(task) }, rstd::move(response) };
}

template<class Entry>
void TaskOwner::finish(Vec<Entry>& entries, i64 handle) {
    for (usize index {}; index < entries.len(); ++index) {
        if (entries[index].owned.handle != handle) continue;
        auto detached = rstd::move(entries[index].owned.task).detach();
        (void)entries.remove(index);
        return;
    }
}

void TaskOwner::finish(Kind kind, i64 handle) {
    switch (kind) {
    case Kind::Sync: finish(sync_, handle); break;
    case Kind::ItemSync: finish(items_, handle); break;
    case Kind::ProviderRequest: finish(requests_, handle); break;
    case Kind::ProviderChange: finish(changes_, handle); break;
    }
    completions_.remove(handle);
}

void TaskOwner::cancel(Task& task) {
    if (task.cancelled) return;
    task.cancelled = true;
    task.task.abort();
}

template<class Entry>
auto TaskOwner::drain(Vec<Entry> entries, bool respond_cancelled) -> coro<empty> {
    for (auto& entry : entries) {
        (void)co_await rstd::move(entry.owned.task);
        if (respond_cancelled && entry.owned.response) {
            auto sink = Response::sink_for(entry.owned.response.clone());
            (void)co_await sink.respond(entry.owned.response->request_id,
                                        Err(ProcessError::NotAuth()));
        }
    }
    co_return empty {};
}

template<class Entry, class Match>
auto TaskOwner::take_replaced(Vec<Entry>& entries, Match matches) -> Vec<Entry> {
    Vec<Entry> removed;
    for (usize index {}; index < entries.len();) {
        if (matches(entries[index])) {
            ++index;
            continue;
        }
        auto entry = entries.remove(index);
        cancel(entry.owned);
        removed.push(rstd::move(entry));
    }
    return removed;
}

template<class Entry>
auto TaskOwner::find(Vec<Entry>& entries, i64 handle) -> Task* {
    for (auto& entry : entries)
        if (entry.owned.handle == handle) return &entry.owned;
    return nullptr;
}

auto TaskOwner::find(i64 handle) -> Task* {
    if (auto task = find(sync_, handle)) return task;
    if (auto task = find(items_, handle)) return task;
    if (auto task = find(requests_, handle)) return task;
    return find(changes_, handle);
}

auto TaskOwner::wait_owned(rstd::async::watch::Receiver<bool> completed) -> coro<empty> {
    while (! *completed.borrow())
        if ((co_await completed.changed()).is_err()) break;
    co_return empty {};
}

auto TaskOwner::finished() -> coro<empty> { co_return empty {}; }

template<class Entry>
void TaskOwner::observe(const Vec<Entry>&                        entries,
                        Vec<rstd::async::watch::Receiver<bool>>& waiting) const {
    for (const auto& entry : entries)
        if (auto completion = completions_.get(entry.owned.handle))
            waiting.push((*completion)->subscribe());
}

auto TaskOwner::wait_all(Vec<rstd::async::watch::Receiver<bool>> waiting) -> coro<empty> {
    for (auto& completion : waiting) co_await wait_owned(rstd::move(completion));
    co_return empty {};
}

auto TaskOwner::make() -> Rc<TaskOwner> {
    auto owner         = make_rc<TaskOwner>();
    owner.get()->self_ = owner.downgrade();
    return owner;
}

auto TaskOwner::start_sync(i64 provider_id, ProviderHandle instance, coro<empty> body)
    -> ProcessResult<i64> {
    if (sync_stopping_) return Err(ProcessError::internal("task owner is stopping"_str));
    for (const auto& entry : sync_)
        if (entry.provider_id == provider_id) {
            if (! entry.owned.cancelled && entry.instance.get() == instance.get() &&
                (! instance || instance->lifecycle_epoch() == entry.lifecycle))
                return Ok(entry.owned.handle);
            return Err(ProcessError::internal("previous sync is stopping"_str));
        }
    auto handle    = rstd_try(next(sync_stopping_));
    auto lifecycle = instance ? instance->lifecycle_epoch() : u64();
    sync_.push(SyncTask { spawn(Kind::Sync, handle, rstd::move(body)),
                          provider_id,
                          rstd::move(instance),
                          lifecycle });
    return Ok(handle);
}

auto TaskOwner::cancel(i64 handle) -> bool {
    auto task = find(handle);
    if (task == nullptr) return false;
    for (const auto& entry : changes_)
        if (entry.owned.handle == handle && ! *entry.cancellable) return false;
    cancel(*task);
    return true;
}

auto TaskOwner::wait(i64 handle) -> coro<empty> {
    if (auto completion = completions_.get(handle))
        return wait_owned((*completion)->subscribe());
    return finished();
}

void TaskOwner::cancel_sync() {
    sync_stopping_ = true;
    for (auto& entry : sync_) cancel(entry.owned);
    for (auto& entry : items_) cancel(entry.owned);
    for (auto& entry : requests_) cancel(entry.owned);
}

auto TaskOwner::stop_sync() -> coro<empty> {
    cancel_sync();
    Vec<rstd::async::watch::Receiver<bool>> waiting;
    observe(sync_, waiting);
    observe(items_, waiting);
    observe(requests_, waiting);
    return wait_all(rstd::move(waiting));
}

void TaskOwner::begin_provider_shutdown() {
    changes_stopping_ = true;
    for (auto& entry : changes_)
        if (*entry.cancellable) cancel(entry.owned);
}

auto TaskOwner::stop_provider_changes() -> coro<empty> {
    begin_provider_shutdown();
    Vec<rstd::async::watch::Receiver<bool>> waiting;
    observe(changes_, waiting);
    return wait_all(rstd::move(waiting));
}

auto TaskOwner::cancel_replaced_requests(ProviderRuntime& runtime) -> coro<empty> {
    auto items    = take_replaced(items_, [&](const auto& entry) {
        return entry.instance && runtime.find(entry.provider_id) == entry.instance.get() &&
               entry.instance->lifecycle_epoch() == entry.lifecycle;
    });
    auto requests = take_replaced(requests_, [&](const auto& entry) {
        return ! entry.owner->instance ||
               runtime.find(entry.owner->provider_id) == entry.owner->instance.get();
    });
    co_await drain(rstd::move(items), true);
    co_await drain(rstd::move(requests), true);
    co_return empty {};
}

auto TaskOwner::cancel_replaced_sync(ProviderRuntime& runtime) -> coro<Vec<i64>> {
    Vec<i64> reset;
    auto     removed = take_replaced(sync_, [&](const auto& entry) {
        if (entry.instance && runtime.find(entry.provider_id) == entry.instance.get() &&
            entry.instance->lifecycle_epoch() == entry.lifecycle)
            return true;
        if (! entry.owned.cancelled && runtime.find(entry.provider_id) != nullptr)
            reset.push(i64(entry.provider_id));
        return false;
    });
    co_await drain(rstd::move(removed));
    co_return reset;
}
} // namespace qcm::backend
