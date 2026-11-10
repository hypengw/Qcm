module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.batch;
export import luato;
export import :provider.dto;
export import :provider.lua_error;
export import :db.sync_batch.batch;
export import :db.error.provider;
import :db.sync_batch.item_reference;
import :db.sync.run;
import :db.timestamp;
export import :model.library_codec;
export import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
namespace lua_batch
{
template<class E>
auto write_error(E error, ref<str> context) -> luato::Error {
    return provider_error_to_lua(rstd::into<ProviderError>(rstd::move(error)).with_context(context),
                                 "qcm.run"_str);
}
} // namespace lua_batch

auto read_run_libraries(sqlito::Connection& db, i64 provider)
    -> Result<Vec<Library>, luato::Error> {
    auto rows = read_libraries(db, provider);
    if (rows.is_err())
        return Err(
            lua_batch::write_error(rstd::move(rows).unwrap_err(), "library query failed"_str));
    return Ok(rstd::move(rows).unwrap());
}
auto resolve_run_items(sqlito::Connection& db, i64 provider, ItemType kind, slice<String> ids)
    -> Result<Vec<Item>, luato::Error> {
    if (ids.len() > usize(500))
        return Err(luato::Error::make(luato::ErrorKind::Type,
                                      String::make("qcm.run"_str),
                                      String::make("invalid resolve_items arguments"_str)));
    Vec<Item> values;
    for (const auto& id : ids) {
        auto item = resolve_unique(db, provider, id.as_str(), kind);
        if (item.is_err()) {
            return Err(lua_batch::write_error(rstd::move(item).unwrap_err(),
                                              "item resolution failed"_str));
        }
        values.push(rstd::move(item).unwrap());
    }
    return Ok(rstd::move(values));
}
class LuaBatchGuard final : public SyncGuard {
    const SyncGuard& origin_;
    const SyncGuard& boundary_;

public:
    LuaBatchGuard(const SyncGuard& origin, const SyncGuard& boundary)
        : origin_(origin), boundary_(boundary) {}
    auto check_origin() const -> Result<empty, sqlito::Error> override {
        rstd_try(boundary_.check_origin());
        return origin_.check_origin();
    }
    auto favorite_changed_after(i64 item) const -> bool override {
        return origin_.favorite_changed_after(item);
    }
};
auto write_run_batch(sqlito::Connection& db, SyncCoordinator& coordinator, i64 provider,
                     ProviderSyncSession* session, const SyncBatch& parsed,
                     const SyncGuard& boundary) -> Result<SyncResult, luato::Error> {
    rstd_try(boundary.check_origin(), [](auto error) {
        return lua_batch::write_error(rstd::move(error), "sync batch origin changed"_str);
    });
    auto locked = session != nullptr ? session->write() : coordinator.write(provider);
    if (locked.is_err())
        return Err(lua_batch::write_error(rstd::move(locked).unwrap_err(),
                                          "sync write guard rejected"_str));
    auto          guard = rstd::move(locked).unwrap();
    LuaBatchGuard checked(*guard, boundary);
    rstd_try(checked.check_origin(), [](auto error) {
        return lua_batch::write_error(rstd::move(error), "sync batch origin changed"_str);
    });
    auto begun = db.begin();
    if (begun.is_err())
        return Err(
            lua_batch::write_error(rstd::move(begun).unwrap_err(), "cannot begin transaction"_str));
    auto tx      = rstd::move(begun).unwrap();
    auto written = sync_batch(tx, provider, parsed, checked, usize());
    if (written.is_err()) {
        return Err(lua_batch::write_error(rstd::move(written).unwrap_err(),
                                          "sync batch write failed"_str));
    }
    auto origin = checked.check_origin();
    if (origin.is_err())
        return Err(lua_batch::write_error(rstd::move(origin).unwrap_err(),
                                          "sync batch origin changed"_str));
    auto committed = tx.commit();
    if (committed.is_err())
        return Err(lua_batch::write_error(rstd::move(committed).unwrap_err(),
                                          "cannot commit sync batch"_str));
    coordinator.bump();
    return Ok(rstd::move(written).unwrap());
}
} // namespace qcm::backend
