module;
#include <rstd/macro.hpp>
export module qcm.backend:task;
export import :event;
export import :provider.item;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::collections::HashMap;
using namespace rstd::prelude;

export namespace qcm::backend
{
class TaskOwner {
    using CompletionSender = rstd::async::watch::Sender<bool>;
    enum class Kind
    {
        Sync,
        ItemSync,
        ProviderRequest,
        ProviderChange
    };
    struct Response {
        ResponseSink sink;
        i32          request_id;
        bool         claimed {};

        static auto ignored() -> coro<ProcessResult<empty>>;
        static auto sink_for(Rc<Response> response) -> ResponseSink;
    };
    struct Task {
        i64                      handle;
        AbortOnDropHandle<empty> task;
        Rc<Response>             response;
        bool                     cancelled {};
    };
    struct SyncTask {
        Task           owned;
        i64            provider_id;
        ProviderHandle instance;
        u64            lifecycle;
    };
    struct ItemSyncTask {
        Task           owned;
        i64            provider_id;
        ProviderHandle instance;
        u64            lifecycle;
    };
    struct ProviderRequestTask {
        Task                       owned;
        ProviderRequestOwnerHandle owner;
    };
    struct ProviderChangeTask {
        Task                 owned;
        ProviderCancelHandle cancellable;
    };
    Vec<SyncTask>                  sync_;
    Vec<ItemSyncTask>              items_;
    Vec<ProviderRequestTask>       requests_;
    Vec<ProviderChangeTask>        changes_;
    HashMap<i64, CompletionSender> completions_;
    alloc::rc::Weak<TaskOwner>     self_;
    long long                      next_handle_ { 1 };
    bool                           sync_stopping_ {};
    bool                           changes_stopping_ {};

    struct Completion;

    auto next(bool stopping) -> ProcessResult<i64>;
    static auto run(Completion completion, coro<empty> body) -> coro<empty>;
    auto spawn(Kind kind, i64 handle, coro<empty> body, Rc<Response> response = {}) -> Task;
    template<class Entry>
    static void finish(Vec<Entry>& entries, i64 handle);
    void finish(Kind kind, i64 handle);
    static void cancel(Task& task);
    template<class Entry>
    static auto drain(Vec<Entry> entries, bool respond_cancelled = false) -> coro<empty>;
    template<class Entry, class Match>
    static auto take_replaced(Vec<Entry>& entries, Match matches) -> Vec<Entry>;
    template<class Entry>
    static auto find(Vec<Entry>& entries, i64 handle) -> Task*;
    auto find(i64 handle) -> Task*;
    static auto wait_owned(rstd::async::watch::Receiver<bool> completed) -> coro<empty>;
    static auto finished() -> coro<empty>;
    template<class Entry>
    void observe(const Vec<Entry>&                        entries,
                 Vec<rstd::async::watch::Receiver<bool>>& waiting) const;
    static auto wait_all(Vec<rstd::async::watch::Receiver<bool>> waiting) -> coro<empty>;

public:
    static auto make() -> Rc<TaskOwner>;
    auto len() const -> usize { return completions_.len(); }
    auto start_sync(i64 provider_id, ProviderHandle instance, coro<empty> body)
        -> ProcessResult<i64>;
    template<class F>
    auto start_item_sync(i64 provider_id, ProviderHandle instance, i32 request_id,
                         ResponseSink sink, F body) -> ProcessResult<i64> {
        auto handle    = rstd_try(next(sync_stopping_));
        auto response  = make_rc<Response>(Response { rstd::move(sink), request_id });
        auto running   = body(Response::sink_for(response.clone()));
        auto lifecycle = instance ? instance->lifecycle_epoch() : u64();
        items_.push(
            ItemSyncTask { spawn(Kind::ItemSync, handle, rstd::move(running), rstd::move(response)),
                           provider_id,
                           rstd::move(instance),
                           lifecycle });
        return Ok(handle);
    }
    template<class F>
    auto start_provider_request(i32 request_id, ResponseSink sink, F body) -> ProcessResult<i64> {
        auto handle   = rstd_try(next(sync_stopping_));
        auto owner    = make_rc<ProviderRequestOwner>();
        auto response = make_rc<Response>(Response { rstd::move(sink), request_id });
        auto running  = body(owner.clone(), Response::sink_for(response.clone()));
        requests_.push(ProviderRequestTask {
            spawn(Kind::ProviderRequest, handle, rstd::move(running), rstd::move(response)),
            rstd::move(owner) });
        return Ok(handle);
    }
    template<class F>
    auto start_provider_change(i32 request_id, ResponseSink sink, F body) -> ProcessResult<i64> {
        auto handle      = rstd_try(next(changes_stopping_));
        auto cancellable = make_rc<bool>(true);
        auto response    = make_rc<Response>(Response { rstd::move(sink), request_id });
        auto running     = body(cancellable.clone(), Response::sink_for(response.clone()));
        changes_.push(ProviderChangeTask {
            spawn(Kind::ProviderChange, handle, rstd::move(running), rstd::move(response)),
            rstd::move(cancellable) });
        return Ok(handle);
    }
    auto cancel(i64 handle) -> bool;
    auto wait(i64 handle) -> coro<empty>;
    void cancel_sync();
    auto stop_sync() -> coro<empty>;
    void begin_provider_shutdown();
    auto stop_provider_changes() -> coro<empty>;
    auto cancel_replaced_requests(ProviderRuntime& runtime) -> coro<empty>;
    auto cancel_replaced_sync(ProviderRuntime& runtime) -> coro<Vec<i64>>;
};
} // namespace qcm::backend
