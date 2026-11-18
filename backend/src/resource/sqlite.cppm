module;
#include <rstd/macro.hpp>
export module qcm.backend:resource.sqlite;
export import rstd;
export import sqlito;
export import liorm.sqlito;
export import :resource.error;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
auto failure(ref<str> text) -> sqlito::Error;
template<class T>
using SqlResult = Result<T, sqlito::Error>;
template<class Statement>
inline auto next(Statement& row) -> SqlResult<bool> {
    return Ok(rstd_try(row.step()) == sqlito::Step::Row);
}
template<class T>
struct Parameter {
    ref<str> name;
    const T& value;
};
template<class T>
auto parameter(ref<str> name, const T& value) -> Parameter<T> {
    return { name, value };
}
auto binding_error(liorm::BindError<sqlito::Error> error) -> Error;
class Database {
    sqlito::Connection*         connection_ {};
    Option<sqlito::Transaction> transaction_;
    auto                        raw_query(ref<str> sql) -> SqlResult<sqlito::Statement>;
    auto                        exec(ref<str> sql) -> SqlResult<empty>;

public:
    void attach(sqlito::Connection& db) { connection_ = rstd::addressof(db); }
    auto open_blob(ref<str> table, ref<str> column, i64 row_id, sqlito::BlobMode mode)
        -> SqlResult<sqlito::Blob>;
    template<class Query, class... Args>
        requires(rstd::mtp::same_as<Query, liorm::query::CompiledQuery> ||
                 rstd::mtp::same_as<Query, liorm::query::ExpandedQuery>)
    auto query(Query query, const Args&... args)
        -> Result<liorm::sqlito::PreparedQuery<Query>, Error> {
        auto          row = rstd_try(transaction_.is_some()
                                         ? liorm::sqlito::prepare(*transaction_, rstd::move(query))
                                         : liorm::sqlito::prepare(*connection_, rstd::move(query)));
        Option<Error> error;
        auto          one = [&](const auto& value) {
            if (error.is_some()) return;
            auto result = row.bind(value.name, value.value);
            if (result.is_err()) error = Some(binding_error(rstd::move(result).unwrap_err()));
        };
        (one(args), ...);
        if (error.is_some()) return Err(rstd::move(*error));
        return Ok(rstd::move(row));
    }
    template<class Query, class... Args>
    auto execute(Query query, const Args&... args) -> Result<empty, Error> {
        auto row = rstd_try(this->query(rstd::move(query), args...));
        rstd_try(liorm::sqlito::discard(row));
        return Ok(empty {});
    }
    auto checkpoint() -> SqlResult<empty>;
    template<class F>
    auto atomic(F function) -> decltype(function()) {
        transaction_ = Some(rstd_try(connection_->begin(sqlito::TransactionMode::Immediate)));
        auto result  = function();
        if (result.is_err()) {
            (void)transaction_->rollback();
            transaction_ = None();
            return result;
        }
        auto committed = transaction_->commit();
        transaction_   = None();
        if (committed.is_err()) return Err(rstd::move(committed).unwrap_err());
        return result;
    }
    auto migrate() -> SqlResult<empty>;
};
} // namespace qcm::backend::resource_cache
