module;
#include <rstd/macro.hpp>
export module qcm.backend:db.provider.favorite;
export import :db.provider.item;
export import :db.sync.run;
import :db.provider.schema;
import :db.state.favorite;
import :model.item;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace provider_favorite_query = liorm::query;

namespace qcm::backend
{
inline constexpr auto favorite_session_query =
    provider_favorite_query::compile([](provider_favorite_query::QueryBuilder& b) {
        auto provider = b.use(provider_tables).table<ProviderRecord>();
        return b.select(provider.col<&ProviderRecord::provider_id>())
            .from(provider.source())
            .where((provider.col<&ProviderRecord::provider_id>() == b.param<i64>("provider"_str)) &&
                   (provider.col<&ProviderRecord::session_key>() ==
                    b.param<provider_favorite_query::TextParameter>("key"_str)));
    });
} // namespace qcm::backend

export namespace qcm::backend
{
struct ProviderFavoriteWrite {
    ProviderItemRead source;
    ProviderItem     target;
    String           session_key;
    bool             value;
};
auto write_provider_favorite(sqlito::Connection& db, SyncCoordinator& coordinator,
                             ProviderFavoriteWrite input) -> ProcessResult<empty> {
    auto& target  = input.target;
    auto  guard   = rstd_try(coordinator.write(target.provider_id));
    auto  session = rstd_try(liorm::sqlito::prepare<favorite_session_query>(
        db,
        liorm::parameters(liorm::param<"provider">(target.provider_id),
                          liorm::param<"key">(input.session_key.as_str()))));
    if (rstd_try(session.step()) != sqlito::Step::Row) return Err(ProcessError::NotAuth());
    auto current = rstd_try(input.source(db));
    if (current.is_none() || current->id != target.id ||
        current->provider_id != target.provider_id || current->type != target.type ||
        current->native_id != target.native_id)
        return Err(ProcessError::NotAuth());
    const bool local = target.type == ItemType::Album || target.type == ItemType::Artist ||
                       target.type == ItemType::Song;
    if (local) rstd_try(write_favorite(db, target.id, input.value));
    guard->favorite(target.id);
    coordinator.bump();
    return Ok(empty {});
}
} // namespace qcm::backend
