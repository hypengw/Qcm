module;
#include <rstd/macro.hpp>
module qcm.backend;
import :db.query.radio;
import :model.item;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::collections::HashSet;

namespace qcm::backend
{
auto radio_queue_row(sqlito::Connection& db, i64 id) -> QueryResult<Option<RadioQueueRow>> {
    auto row = rstd_try(liorm::sqlito::fetch_one<radio_queue_find_query>(
        db,
        radio_queue_projection,
        liorm::parameters(liorm::param<"id">(id),
                          liorm::param<"type">(i64(static_cast<int>(ItemType::Radio)))),
        liorm::sqlito::RowConsumption::First));
    if (row.is_none()) return Ok(None<RadioQueueRow>());
    return Ok(Some(RadioQueueRow { id,
                                   row->provider_id.unwrap_or(i64()),
                                   row->name.unwrap_or(String::make()),
                                   row->description.unwrap_or(String::make()),
                                   row->native_id.unwrap_or(String::make()) }));
}

auto radio_queue_rows(sqlito::Connection& db, slice<i64> providers)
    -> QueryResult<Vec<RadioQueueRow>> {
    auto               values = rstd_try(liorm::sqlito::fetch_all<radio_queues_query>(
        db,
        radio_queues_projection,
        liorm::parameters(liorm::param<"type">("queue"_str), liorm::param<"item_type">(i64(55)))));
    Vec<RadioQueueRow> rows;
    for (auto& fields : values) {
        auto provider_id = fields.provider_id.unwrap_or(i64());
        bool included    = false;
        for (auto wanted : providers)
            if (wanted == provider_id) {
                included = true;
                break;
            }
        if (included)
            rows.push(RadioQueueRow { fields.id.unwrap_or(i64()),
                                      provider_id,
                                      fields.name.unwrap_or(String::make()),
                                      fields.description.unwrap_or(String::make()),
                                      fields.native_id.unwrap_or(String::make()) });
    }
    return Ok(rstd::move(rows));
}

auto radio_ordered_songs(sqlito::Connection& db, i64 provider, slice<i64> ids)
    -> QueryResult<Vec<i64>> {
    auto         query = rstd_try(liorm::sqlito::prepare<radio_song_query>(
        db, liorm::parameters(liorm::param<"id">(i64()), liorm::param<"provider">(provider))));
    HashSet<i64> seen;
    Vec<i64>     result;
    for (auto id : ids) {
        if (! seen.insert(i64(id))) continue;
        (void)rstd_try(query.reset());
        rstd_try(query.bind(
            liorm::parameters(liorm::param<"id">(id), liorm::param<"provider">(provider))));
        if (rstd_try(liorm::sqlito::row_exists(query))) result.push(i64(id));
    }
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
