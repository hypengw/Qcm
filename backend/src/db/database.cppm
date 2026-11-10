module;
#include <rstd/macro.hpp>
export module qcm.backend:db.database;
import :fts;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
using DbError      = sqlito::Error;
using DbErrorKind  = sqlito::ErrorKind;
using DbStep       = sqlito::Step;
using DbExecutor   = sqlito::Executor;
using DbConnection = sqlito::Connection;

export namespace qcm::backend
{
template<class F>
auto database_run(sqlito::ExecutorHandle db, F function)
    -> rstd::async::coro<decltype(function(rstd::declval<DbConnection&>()))> {
    using Output = decltype(function(rstd::declval<DbConnection&>()));
    auto submitted =
        rstd_co_try(db.submit([function = rstd::move(function)](
                                  DbConnection& connection) mutable -> Result<Output, DbError> {
            return Ok(function(connection));
        }));
    co_return rstd_co_try(co_await rstd::move(submitted));
}

class MainDatabase {
    struct Registrations {
        sqlito::FtsRegistration    fts;
        sqlito::ScalarRegistration query;
    };
    struct Owner {
        Option<sqlito::ScalarRegistration> query;
        Option<sqlito::FtsRegistration>    fts;
        Option<DbExecutor>                 executor;
        Owner(DbExecutor value, Registrations registrations)
            : query(Some(rstd::move(registrations.query))),
              fts(Some(rstd::move(registrations.fts))),
              executor(Some(rstd::move(value))) {}
        Owner(Owner&& other)
            : query(other.query.take()), fts(other.fts.take()), executor(other.executor.take()) {}
        Owner(const Owner&) = delete;
    };
    Option<Owner> owner_;
    explicit MainDatabase(DbExecutor executor, Registrations registrations)
        : owner_(Some(Owner(rstd::move(executor), rstd::move(registrations)))) {}
    static auto shutdown_owned(Option<Owner> owner) -> rstd::async::coro<Result<empty, DbError>> {
        if (owner.is_none())
            co_return Err(DbError::from(
                DbErrorKind::State, 0, String::make("database shutdown already requested"_str)));
        auto owned = rstd::move(owner).unwrap();
        co_return co_await owned.executor->shutdown();
    }
    static auto initialize(DbConnection& db, bool memory) -> Result<Registrations, DbError> {
        rstd_try(db.execute_batch(
            "PRAGMA foreign_keys=ON; PRAGMA busy_timeout=5000; PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;"_str));
        auto fk   = rstd_try(db.prepare("PRAGMA foreign_keys"_str));
        auto step = rstd_try(fk.step());
        if (step != DbStep::Row)
            return Err(DbError::from(
                DbErrorKind::State, 0, String::make("foreign key pragma missing"_str)));
        auto enabled = rstd_try(fk.integer(usize()));
        if (enabled.is_none() || *enabled != i64(1))
            return Err(
                DbError::from(DbErrorKind::State, 0, String::make("foreign keys disabled"_str)));
        rstd_try(fk.step());
        auto journal = rstd_try(db.prepare("PRAGMA journal_mode"_str));
        if (rstd_try(journal.step()) != DbStep::Row)
            return Err(
                DbError::from(DbErrorKind::State, 0, String::make("journal mode missing"_str)));
        auto mode = rstd_try(journal.text(usize()));
        if (mode.is_none() || mode->as_str() != (memory ? "memory"_str : "wal"_str))
            return Err(
                DbError::from(DbErrorKind::State, 0, String::make("unexpected journal mode"_str)));
        rstd_try(journal.step());
        auto timeout = rstd_try(db.prepare("PRAGMA busy_timeout"_str));
        if (rstd_try(timeout.step()) != DbStep::Row)
            return Err(
                DbError::from(DbErrorKind::State, 0, String::make("busy timeout missing"_str)));
        auto value = rstd_try(timeout.integer(usize()));
        if (value.is_none() || *value != i64(5000))
            return Err(
                DbError::from(DbErrorKind::State, 0, String::make("unexpected busy timeout"_str)));
        rstd_try(timeout.step());
        auto synchronous = rstd_try(db.prepare("PRAGMA synchronous"_str));
        if (rstd_try(synchronous.step()) != DbStep::Row)
            return Err(
                DbError::from(DbErrorKind::State, 0, String::make("synchronous mode missing"_str)));
        auto sync_mode = rstd_try(synchronous.integer(usize()));
        if (sync_mode.is_none() || *sync_mode != i64(1))
            return Err(DbError::from(
                DbErrorKind::State, 0, String::make("unexpected synchronous mode"_str)));
        rstd_try(synchronous.step());
        auto fts = rstd_try(
            db.register_or_replace_fts5_tokenizer(String::make("qcm"_str), qcm_fts_factory()));
        auto query = rstd_try(db.register_or_replace_scalar_function(
            String::make("qcm_query"_str), qcm_fts_scalar(), true));
        return Ok(Registrations { rstd::move(fts), rstd::move(query) });
    }

public:
    MainDatabase(MainDatabase&& other): owner_(other.owner_.take()) {}
    MainDatabase(const MainDatabase&) = delete;
    static auto open(String path) -> rstd::async::coro<Result<MainDatabase, DbError>> {
        bool memory   = path.as_str() == ":memory:"_str;
        auto executor = rstd_co_try(DbExecutor::open(path.as_str()));
        auto op       = rstd_co_try(executor.handle().submit([memory](DbConnection& db) {
            return initialize(db, memory);
        }));
        auto result   = rstd_co_try(co_await rstd::move(op));
        co_return Ok(MainDatabase(rstd::move(executor), rstd::move(result)));
    }
    auto handle() const -> Result<sqlito::ExecutorHandle, DbError> {
        if (owner_.is_none())
            return Err(
                DbError::from(DbErrorKind::State, 0, String::make("database is shut down"_str)));
        return Ok(owner_->executor->handle());
    }
    auto shutdown() -> rstd::async::coro<Result<empty, DbError>> {
        return shutdown_owned(owner_.take());
    }
};
} // namespace qcm::backend
