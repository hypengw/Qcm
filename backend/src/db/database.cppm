module;
#include <rstd/macro.hpp>
export module qcm.backend:db.database;
export import sqlito;
using namespace rstd::prelude;
using DbError      = sqlito::Error;
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
        Owner(DbExecutor value, Registrations registrations);
        Owner(Owner&& other);
        Owner(const Owner&) = delete;
    };
    Option<Owner> owner_;
    explicit MainDatabase(DbExecutor executor, Registrations registrations);
    static auto shutdown_owned(Option<Owner> owner) -> rstd::async::coro<Result<empty, DbError>>;
    static auto initialize(DbConnection& db, bool memory) -> Result<Registrations, DbError>;

public:
    MainDatabase(MainDatabase&& other);
    MainDatabase(const MainDatabase&) = delete;
    static auto open(String path) -> rstd::async::coro<Result<MainDatabase, DbError>>;
    auto handle() const -> Result<sqlito::ExecutorHandle, DbError>;
    auto shutdown() -> rstd::async::coro<Result<empty, DbError>>;
};
} // namespace qcm::backend
