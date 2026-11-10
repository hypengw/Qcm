export module qcm.backend:provider.context;
export import :provider.dto;
export import :db.sync.run;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;

export namespace qcm::backend
{
class ProviderContext {
    using Predicate = Box<dyn<Fn<bool()>>>;
    using Progress  = Box<dyn<Fn<coro<empty>(SyncProgressKind, i32)>>>;
    Option<Rc<Predicate>> valid_;
    Option<Rc<Predicate>> writable_;
    Option<Rc<Progress>>  progress_;
    static auto progress_owned(Option<Rc<Progress>> callback, SyncProgressKind kind, i32 count)
        -> coro<empty> {
        if (callback.is_some()) co_await (**callback)->operator()(kind, count);
        co_return empty {};
    }

public:
    sqlito::ExecutorHandle db;
    SyncCoordinator        coordinator;
    i64                    provider_id;
    ProviderSyncConfig     sync;
    ProviderContext(sqlito::ExecutorHandle database, SyncCoordinator coordination, i64 provider,
                    ProviderSyncConfig config = {})
        : db(rstd::move(database)),
          coordinator(rstd::move(coordination)),
          provider_id(provider),
          sync(rstd::move(config)) {}
    auto clone() const -> ProviderContext {
        ProviderContext copy(db.clone(), coordinator.clone(), provider_id, sync.clone());
        if (valid_.is_some()) copy.valid_ = Some(valid_->clone());
        if (writable_.is_some()) copy.writable_ = Some(writable_->clone());
        if (progress_.is_some()) copy.progress_ = Some(progress_->clone());
        return copy;
    }
    template<class F>
    void set_valid(F value) {
        valid_ = Some(make_rc<Predicate>(Predicate::make(rstd::move(value))));
    }
    template<class F>
    void restrict_valid(F value) {
        set_valid([previous = valid_.take(), extra = rstd::move(value)] {
            return (previous.is_none() || (**previous)->operator()()) && extra();
        });
    }
    template<class F>
    void set_writable(F value) {
        writable_ = Some(make_rc<Predicate>(Predicate::make(rstd::move(value))));
    }
    template<class F>
    void set_progress(F value) {
        progress_ = Some(make_rc<Progress>(Progress::make(rstd::move(value))));
    }
    auto is_valid() const -> bool { return valid_.is_none() || (**valid_)->operator()(); }
    auto is_writable() const -> bool {
        return is_valid() && (writable_.is_none() || (**writable_)->operator()());
    }
    auto progress(SyncProgressKind kind, i32 count) const -> coro<empty> {
        return progress_owned(progress_.is_some() ? Some(progress_->clone()) : None(), kind, count);
    }
};
} // namespace qcm::backend
