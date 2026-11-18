module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :db.sync_batch;
import liorm.sqlito;
import :db.codec;
import :model.remote_mix;
import :model.rel_mix_song;
import :db.schema;
import :db.timestamp;
using rstd::collections::HashSet;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace contents_query = liorm::query;

namespace qcm::backend
{
namespace
{
inline constexpr contents_query::ProjectionLayout<contents_query::ModelGroup<Mix>>
                      contents_cache_projection;
inline constexpr auto contents_find_cache =
    contents_query::compile([](contents_query::QueryBuilder& b) {
        auto mix = b.use(database_tables).table<Mix>();
        return b.select(contents_query::project(contents_cache_projection, mix).as_slice())
            .from(mix.source())
            .where(mix.col<&Mix::remote_id>() == b.param<i64>("remote_id"_str))
            .limit(b.integer(1));
    });
inline constexpr auto contents_delete =
    contents_query::compile([](contents_query::QueryBuilder& b) {
        auto relation = b.use(database_tables).table<RelMixSong>();
        return b.delete_from(relation.source())
            .where(relation.col<&RelMixSong::mix_id>() == b.param<i64>("mix_id"_str));
    });
inline constexpr auto contents_upsert =
    contents_query::compile([](contents_query::QueryBuilder& b) {
        auto rel = b.use(database_tables).table<RelMixSong>();
        return b
            .insert_into(rel.source(),
                         rel.col<&RelMixSong::mix_id>(),
                         rel.col<&RelMixSong::song_id>(),
                         rel.col<&RelMixSong::order_idx>(),
                         rel.col<&RelMixSong::update_at>())
            .values(b.param<i64>("mix_id"_str),
                    b.param<i64>("song_id"_str),
                    b.param<i64>("order"_str),
                    b.param<Timestamp>("updated"_str))
            .on_conflict_do_update(
                array<contents_query::Expr, 2> { rel.col<&RelMixSong::mix_id>(),
                                                 rel.col<&RelMixSong::song_id>() }
                    .as_slice(),
                array<contents_query::Expr, 2> { rel.col<&RelMixSong::order_idx>().set(
                                                     b.excluded(rel.col<&RelMixSong::order_idx>())),
                                                 rel.col<&RelMixSong::update_at>().set(b.excluded(
                                                     rel.col<&RelMixSong::update_at>())) }
                    .as_slice());
    });
auto contents_cache(sqlito::Transaction& tx, i64 remote_id) -> DbWriteResult<Mix> {
    auto found = rstd_try(liorm::sqlito::fetch_model<contents_find_cache>(
        tx,
        contents_cache_projection.group<0>(),
        liorm::parameters(liorm::param<"remote_id">(remote_id))));
    if (found.is_none()) return Err(DbWriteError::Invalid(String::make("missing cache mix"_str)));
    return Ok(rstd::move(found).unwrap());
}
} // namespace

auto write_legacy_mix_songs(sqlito::Transaction& tx, i64 provider, i64 remote_id, slice<i64> songs)
    -> DbWriteResult<empty> {
    using C = liorm::Columns<Mix>;
    array<ItemType, 2> mix_types { ItemType::Mix, ItemType::Radio };
    array<ItemType, 1> song_types { ItemType::Song };
    rstd_try(resolve(tx, provider, ItemRef::Id(remote_id), mix_types.as_slice()));
    auto mix = rstd_try(contents_cache(tx, remote_id));
    for (auto id : songs) {
        rstd_try(resolve(tx, provider, ItemRef::Id(id), song_types.as_slice()));
    }
    auto                  now = Timestamp::now();
    liorm::Changeset<Mix> changes { C::update_at = now, C::content_update_at = now };
    rstd_try(liorm::sqlito::update<database_tables>(tx, mix.id, rstd::move(changes)));
    auto  stmt = rstd_try(liorm::sqlito::prepare(tx, contents_upsert));
    usize index {};
    for (auto id : songs) {
        rstd_try(stmt.bind("mix_id"_str, mix.id));
        rstd_try(stmt.bind("song_id"_str, id));
        rstd_try(stmt.bind("order"_str, i64(static_cast<long long>(index.to_primitive()))));
        rstd_try(stmt.bind("updated"_str, now));
        rstd_try(liorm::sqlito::discard(stmt));
        rstd_try(stmt.reset());
        ++index;
    }
    return Ok(empty {});
}

auto write_remote_mix_contents(sqlito::Transaction& tx, i64 provider, slice<MixContents> entries)
    -> DbWriteResult<empty> {
    HashSet<i64>       mixes;
    array<ItemType, 2> mix_types { ItemType::Mix, ItemType::Radio };
    array<ItemType, 1> song_types { ItemType::Song };
    for (const auto& entry : entries) {
        if (entry.mode != RelationMode::Replace)
            return Err(
                DbWriteError::Invalid(String::make("ordered mix contents require replace"_str)));
        auto remote = rstd_try(resolve(tx, provider, entry.mix, mix_types.as_slice()));
        if (! mixes.insert(remote.id))
            return Err(DbWriteError::Invalid(String::make("duplicate mix contents"_str)));
        auto         mix = rstd_try(contents_cache(tx, remote.id));
        HashSet<i64> unique;
        Vec<i64>     songs;
        // Validate this entire entry before its DELETE, but do not prevalidate
        // later entries: an error there must leave earlier writes in this tx.
        for (const auto& reference : entry.songs.as_slice()) {
            auto song = rstd_try(resolve(tx, provider, reference, song_types.as_slice()));
            if (! unique.insert(song.id))
                return Err(DbWriteError::Invalid(String::make("duplicate_member"_str)));
            songs.push(i64(song.id));
        }
        {
            auto stmt = rstd_try(liorm::sqlito::prepare<contents_delete>(
                tx, liorm::parameters(liorm::param<"mix_id">(mix.id))));
            rstd_try(liorm::sqlito::discard(stmt));
        }
        usize order {};
        for (auto song : songs.as_slice()) {
            using C = liorm::Columns<RelMixSong>;
            liorm::InsertValues<RelMixSong> values { C::song_id   = song,
                                                     C::mix_id    = mix.id,
                                                     C::order_idx = i64(static_cast<long long>(
                                                         order.to_primitive())),
                                                     C::update_at = Timestamp::now() };
            // Do not bulk these inserts: a later failure leaves prior rows in tx.
            rstd_try(liorm::sqlito::insert<database_tables>(tx, rstd::move(values)));
            ++order;
        }
        auto count = i32(static_cast<rstd::int32_t>(songs.len().to_primitive()));
        {
            using C = liorm::Columns<RemoteMix>;
            liorm::Changeset<RemoteMix> changes { C::track_count = count };
            rstd_try(liorm::sqlito::update<database_tables>(tx, remote.id, rstd::move(changes)));
        }
        using C                   = liorm::Columns<Mix>;
        auto                  now = Timestamp::now();
        liorm::Changeset<Mix> changes { C::track_count       = count,
                                        C::update_at         = now,
                                        C::content_update_at = now };
        rstd_try(liorm::sqlito::update<database_tables>(tx, mix.id, rstd::move(changes)));
    }
    return Ok(empty {});
}
} // namespace qcm::backend
