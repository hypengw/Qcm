module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.provider.item;
import :db.schema;
import :model.mix_model;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace provider_item_query = liorm::query;

namespace qcm::backend
{
namespace
{
struct ProviderItemFields {
    i64      provider_id;
    String   native_id;
    ItemType type;
};
inline constexpr liorm::RecordProjection<ProviderItemFields, &ProviderItemFields::provider_id,
                                         &ProviderItemFields::native_id, &ProviderItemFields::type>
                      provider_item_projection;
inline constexpr auto provider_item_row =
    provider_item_query::compile([](provider_item_query::QueryBuilder& b) {
        auto item = b.use(database_tables).table<Item>();
        return provider_item_projection
            .select(b,
                    item.col<&Item::provider_id>(),
                    item.col<&Item::native_id>(),
                    item.col<&Item::type>())
            .from(item.source())
            .where(item.col<&Item::id>() == b.param<i64>("id"_str));
    });
inline constexpr auto provider_mix_remote =
    provider_item_query::compile([](provider_item_query::QueryBuilder& b) {
        auto mix = b.use(database_tables).table<Mix>();
        return b.select(mix.col<&Mix::remote_id>())
            .from(mix.source())
            .where(mix.col<&Mix::id>() == b.param<i64>("id"_str));
    });
} // namespace

auto ProviderItemRead::operator()(sqlito::Connection& db) const -> ProcessResult<Option<ProviderItem>> {
    auto id   = id_;
    auto kind = type_;
    if (kind == ItemType::Mix) {
        auto remote =
            rstd_try((liorm::sqlito::fetch_optional_scalar<provider_mix_remote, Option<i64>>(
                db,
                liorm::parameters(liorm::param<"id">(id)),
                liorm::sqlito::RowConsumption::First)));
        if (remote.is_none()) return Err(ProcessError::NoSuchMix(rstd::format("{}", id)));
        if (remote->is_none()) return Ok(None());
        id = **remote;
    } else if (kind != ItemType::Song && kind != ItemType::Album && kind != ItemType::Artist)
        return Ok(None());
    auto row = rstd_try(
        liorm::sqlito::fetch_one<provider_item_row>(db,
                                                    provider_item_projection,
                                                    liorm::parameters(liorm::param<"id">(id)),
                                                    liorm::sqlito::RowConsumption::First));
    if (row.is_none()) return Err(ProcessError::NotFound());
    if (row->type != kind) return Err(ProcessError::WrongId(rstd::format("{}", id_)));
    return Ok(
        Some(ProviderItem { id, row->provider_id, rstd::move(row->native_id), row->type }));
}
} // namespace qcm::backend
