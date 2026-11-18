module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :db.sync_batch;
import liorm.sqlito;
import :db.schema;
import :db.codec;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
namespace
{
auto find_song(sqlito::Transaction& tx, i64 id) -> DbWriteResult<Option<Song>> {
    return Ok(rstd_try((liorm::sqlito::find<database_tables, Song>(tx, id))));
}
} // namespace

auto SongTags::convert(const Vec<String>& value) -> rstd::json::Value {
    rstd::json::Array tags;
    for (const auto& tag : value.as_slice()) tags.push(rstd::json::Value::String(tag.clone()));
    return rstd::json::Value::Array(rstd::move(tags));
}

auto write_legacy_songs(sqlito::Transaction& tx, i64 provider, slice<Song> entries)
    -> DbWriteResult<Vec<i64>> {
    array<ItemType, 1> song_type { ItemType::Song };
    array<ItemType, 1> album_type { ItemType::Album };
    Vec<i64>           ids;
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.id), song_type.as_slice()));
        if (entry.album_id.is_some()) {
            rstd_try(resolve(tx, provider, ItemRef::Id(*entry.album_id), album_type.as_slice()));
        }
        auto old = rstd_try(find_song(tx, entry.id));
        Song model { entry.id,         entry.name.clone(), entry.sort_name.clone(),
                     entry.album_id,   entry.track_number, entry.disc_number,
                     entry.duration,   entry.can_play,     entry.tags.clone(),
                     entry.popularity, entry.publish_time, entry.added_at };
        rstd_try(old.is_none() ? liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model))
                               : liorm::sqlito::update_all<database_tables>(tx, rstd::move(model)));
        ids.push(i64(entry.id));
    }
    return Ok(rstd::move(ids));
}

auto write_songs(sqlito::Transaction& tx, i64 provider, slice<SongInput> entries, const Mask& mask)
    -> DbWriteResult<SongWriteOutcome> {
    array<ItemType, 1> expected { ItemType::Song };
    SongWriteOutcome   outcome;
    for (const auto& entry : entries) {
        auto item     = rstd_try(resolve(tx, provider, entry.item, expected.as_slice()));
        auto old      = rstd_try(find_song(tx, item.id));
        auto inserted = old.is_none();
        auto model =
            inserted
                ? Song { item.id,  String::make(), None(),
                         None(),   i32(),          i32(),
                         i64(),    true,           rstd::json::Value::Array(rstd::json::Array {}),
                         f64(0.0), None(),         None() }
                : rstd::move(old).unwrap();
        rstd_try(entry.apply(model, inserted, mask));
        if (model.name.is_empty())
            return Err(DbWriteError::Invalid(String::make("entity name is required"_str)));
        auto written =
            rstd_try(inserted ? liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model))
                              : liorm::sqlito::update_all<database_tables>(tx, rstd::move(model)));
        if (inserted) outcome.inserted.push(i64(item.id));
        outcome.rows.push(rstd::move(written));
    }
    for (const auto& song : outcome.rows.as_slice())
        if (song.duration < i64() || song.track_number < i32() || song.disc_number < i32() ||
            ! song.popularity.is_finite())
            return Err(DbWriteError::Invalid(String::make("invalid song quantity"_str)));
    return Ok(rstd::move(outcome));
}
} // namespace qcm::backend
