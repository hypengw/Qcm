module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:model.mix;
export import liorm;
export import sqlito;
import liorm.sqlito;
export import :model.rel_mix_song;
import :db.timestamp;
import :db.codec;
import :db.schema;
import :db.error.query;
import :model.mix_model;
import :model.song;
import :pagination;
using rstd::tuple;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace mix_query = liorm::query;

export namespace qcm::backend
{
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum MixErrorReason {
        [[error("{error}")]]
        Query([[source]] error : mix_query::Error),
        [[error("{error}")]]
        Storage([[source]] error : sqlito::Error),
        [[error("mix range exceeded")]]
        Range,
        [[error("{error}")]]
        Binding([[source]] error : liorm::BindError<sqlito::Error>),
    })]];
// clang-format on
struct [[pmacro::derive("rstd::Debug")]] MixError {
    MixErrorReason reason;
    bool           outcome_unknown {};
};
template<class T>
using MixResult = Result<T, MixError>;
static_assert(! rstd::mtp::copy<MixError>);
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<convert::From<sqlito::Error>, qcm::backend::MixError> {
    static auto from(sqlito::Error error) -> qcm::backend::MixError {
        return { qcm::backend::MixErrorReason::Storage(rstd::move(error)) };
    }
};
template<>
struct Impl<convert::From<liorm::BindError<sqlito::Error>>, qcm::backend::MixError> {
    static auto from(liorm::BindError<sqlito::Error> error) -> qcm::backend::MixError {
        return { qcm::backend::MixErrorReason::Binding(rstd::move(error)) };
    }
};
template<>
struct Impl<convert::From<liorm::query::Error>, qcm::backend::MixError> {
    static auto from(liorm::query::Error error) -> qcm::backend::MixError {
        return { qcm::backend::MixErrorReason::Query(rstd::move(error)) };
    }
};
template<>
struct Impl<fmt::Display, qcm::backend::MixError> : ImplBase<qcm::backend::MixError> {
    auto fmt(fmt::Formatter& f) const -> bool {
        const auto& error = this->self();
        if (error.outcome_unknown && ! f.write_str("mix write outcome unknown: "_str)) return false;
        return f.write_fmt(fmt::Arguments::make("{}", error.reason));
    }
};
template<>
struct Impl<error::Error, qcm::backend::MixError>
    : DefaultInImpl<error::Error, qcm::backend::MixError> {
    auto source() const noexcept -> Option<error::ErrorRef> {
        return Some(dyn<error::Error>::from_ref(this->self().reason));
    }
};
} // namespace rstd

namespace qcm::backend
{
inline constexpr mix_query::ProjectionLayout<mix_query::ModelGroup<Mix>> mix_projection;
inline constexpr auto mix_find_row = mix_query::compile([](mix_query::QueryBuilder& b) {
    auto t = b.use(database_tables).table<Mix>();
    return b.select(mix_query::project(mix_projection, t).as_slice())
        .from(t.source())
        .where(t.col<&Mix::id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});
template<class Session>
auto find_mix_row(Session& session, i64 id) -> QueryResult<Option<Mix>> {
    auto stmt = rstd_try(liorm::sqlito::prepare(session, mix_find_row));
    rstd_try(stmt.bind("id"_str, id));
    return Ok(rstd_try(liorm::sqlito::one_model(stmt, mix_projection.group<0>())));
}
inline constexpr liorm::InsertRowsShape<liorm::sqlito::Writer, RelMixSong, &RelMixSong::song_id,
                                        &RelMixSong::mix_id, &RelMixSong::order_idx>
                      mix_append_rows;
inline constexpr auto mix_count_update   = mix_query::compile([](mix_query::QueryBuilder& b) {
    auto t = b.use(database_tables).table<Mix>();
    return b.update(t.source())
        .set(t.col<&Mix::track_count>().set(t.col<&Mix::track_count>() + b.param<i64>("delta"_str)),
             t.col<&Mix::update_at>().set(b.param<Timestamp>("now"_str)))
        .where(t.col<&Mix::id>() == b.param<i64>("mix_id"_str));
});
inline constexpr auto mix_pending_count  = mix_query::compile([](mix_query::QueryBuilder& b) {
    auto rel = b.use(database_tables).table<RelMixSong>();
    return b.select(b.function(mix_query::Function::Count, b.star()))
        .from(rel.source())
        .where((rel.col<&RelMixSong::mix_id>() == b.param<i64>("mix_id"_str)) &&
               (rel.col<&RelMixSong::order_idx>() == b.integer(0)));
});
inline constexpr auto mix_reorder_append = mix_query::compile([](mix_query::QueryBuilder& b) {
    auto tables = b.use(database_tables);
    auto old    = tables.table<RelMixSong>();
    auto max_order =
        b.select(b.function(mix_query::Function::Coalesce,
                            b.function(mix_query::Function::Max, old.col<&RelMixSong::order_idx>()),
                            b.integer(-1))
                     .as("max_order_idx"_str))
            .from(old.source())
            .where(old.col<&RelMixSong::mix_id>() == b.param<i64>("mix_id"_str));
    auto pending = tables.table<RelMixSong>();
    auto new_orders =
        b.select(pending.col<&RelMixSong::song_id>(),
                 (b.col("max_order"_str, "max_order_idx"_str) +
                  b.row_number(pending.col<&RelMixSong::id>()))
                     .as("new_order_idx"_str))
            .from(pending.source())
            .join(b.cte("max_order"_str), b.integer(1) == b.integer(1))
            .where((pending.col<&RelMixSong::mix_id>() == b.param<i64>("mix_id"_str)) &&
                   (pending.col<&RelMixSong::order_idx>() == b.integer(0)));
    auto target = tables.table<RelMixSong>();
    auto next_order =
        b.select(b.col("new_orders"_str, "new_order_idx"_str))
            .from(b.cte("new_orders"_str))
            .where(target.col<&RelMixSong::song_id>() == b.col("new_orders"_str, "song_id"_str));
    return b.update(target.source())
        .set(target.col<&RelMixSong::order_idx>().set(b.scalar(next_order)))
        .where((target.col<&RelMixSong::mix_id>() == b.param<i64>("mix_id"_str)) &&
               (target.col<&RelMixSong::order_idx>() == b.integer(0)))
        .with("max_order"_str, max_order)
        .with("new_orders"_str, new_orders);
});
struct MixIdPage {
    Vec<i64> ids;
    i64      total {};
};
enum class MixSongOrder
{
    TrackNumber,
    Title,
    SortTitle,
    PublishTime,
    Duration,
    Popularity,
    Random
};
template<class SongTable>
auto mix_song_column(mix_query::QueryBuilder& b, const SongTable& song, MixSongOrder order)
    -> mix_query::Expr {
    switch (order) {
    case MixSongOrder::TrackNumber: return song.template col<&Song::track_number>();
    case MixSongOrder::Title: return song.template col<&Song::name>();
    case MixSongOrder::SortTitle: return song.template col<&Song::sort_name>();
    case MixSongOrder::PublishTime: return song.template col<&Song::publish_time>();
    case MixSongOrder::Duration: return song.template col<&Song::duration>();
    case MixSongOrder::Popularity: return song.template col<&Song::popularity>();
    case MixSongOrder::Random: return b.function(mix_query::Function::Random);
    }
    return song.template col<&Song::name>();
}
inline constexpr liorm::TupleProjection<i64> mix_song_id_projection;
inline constexpr auto mix_find_id = mix_query::compile([](mix_query::QueryBuilder& b) {
    auto mix = b.use(database_tables).table<Mix>();
    return b.select(mix.col<&Mix::id>())
        .from(mix.source())
        .where(mix.col<&Mix::id>() == b.param<i64>("id"_str));
});
struct MixSongPage {
    Vec<i64> ids;
    i64      total {};
};
template<class Database>
auto mix_song_ids(Database& db, i64 mix_id, i32 page, i32 page_size, MixSongOrder order, bool asc)
    -> QueryResult<MixSongPage> {
    auto                    pagination = page_of(page, page_size);
    mix_query::QueryBuilder b;
    auto                    tables   = b.use(database_tables);
    auto                    song     = tables.table<Song>();
    auto                    relation = tables.table<RelMixSong>();
    auto                    sorted   = mix_song_column(b, song, order);
    auto                    query    = rstd_try(mix_query::try_compile_page(
        mix_song_id_projection.select(b, song.col<&Song::id>())
            .from(song.source())
            .join(relation.source(), relation.col<&RelMixSong::song_id>() == song.col<&Song::id>())
            .where(relation.col<&RelMixSong::mix_id>() == b.param<i64>("mix_id"_str))
            .order_by(asc ? sorted.asc() : sorted.desc(),
                      relation.col<&RelMixSong::order_idx>().desc()),
        i64(pagination.size.to_primitive()),
        pagination.offset));
    auto                    stmt     = rstd_try(liorm::sqlito::prepare_page(db, rstd::move(query)));
    rstd_try(stmt.bind("mix_id"_str, mix_id));
    auto        rows = rstd_try(stmt.fetch(mix_song_id_projection));
    MixSongPage result;
    result.total = rows.total;
    for (auto& row : rows.rows) result.ids.push(rstd::move(rstd::get<0>(row)));
    return Ok(rstd::move(result));
}
template<class Database>
auto mix_find(Database& db, i64 id) -> QueryResult<Option<i64>> {
    auto stmt = rstd_try(liorm::sqlito::prepare(db, mix_find_id));
    rstd_try(stmt.bind("id"_str, id));
    return Ok(rstd_try(
        liorm::sqlito::optional_scalar<i64>(stmt, usize(), liorm::sqlito::RowConsumption::Drain)));
}
auto update_mix_count(sqlito::Transaction& tx, i64 id, i64 delta) -> MixResult<empty> {
    if (delta == i64()) return Ok(empty {});
    auto stmt = rstd_try(liorm::sqlito::prepare(tx, mix_count_update));
    rstd_try(stmt.bind("mix_id"_str, id));
    rstd_try(stmt.bind("delta"_str, delta));
    rstd_try(stmt.bind("now"_str, Timestamp::now()));
    rstd_try(liorm::sqlito::discard(stmt));
    return Ok(empty {});
}
} // namespace qcm::backend
export namespace qcm::backend
{
// The caller owns the transaction and must roll it back after an error.
auto remove_songs(sqlito::Transaction& tx, i64 mix_id, slice<i64> song_ids) -> MixResult<u64> {
    auto query = rstd_try(mix_query::try_compile_expanded_planned([&](mix_query::QueryBuilder& b) {
        auto rel = b.use(database_tables).table<RelMixSong>();
        auto ids = b.param_group<i64>("song_ids"_str, song_ids.len());
        return b.delete_from(rel.source())
            .where((rel.col<&RelMixSong::mix_id>() == b.param<i64>("mix_id"_str)) &&
                   rel.col<&RelMixSong::song_id>().in(ids));
    }));
    u64  affected;
    {
        auto stmt = rstd_try(liorm::sqlito::prepare(tx, rstd::move(query)));
        rstd_try(stmt.bind("mix_id"_str, mix_id));
        rstd_try(stmt.bind_group("song_ids"_str, song_ids));
        rstd_try(liorm::sqlito::discard(stmt));
        affected = rstd_try(tx.last_changes());
    }
    if (affected > u64(9223372036854775807ULL)) return Err(MixError { MixErrorReason::Range() });
    auto delta = i64(-static_cast<long long>(affected.to_primitive()));
    rstd_try(update_mix_count(tx, mix_id, delta));
    return Ok(affected);
}
// Empty input skips only the INSERT: existing zero-order rows still get appended.
// As with remove_songs, the caller owns rollback/commit of this transaction.
auto append_songs(sqlito::Transaction& tx, i64 mix_id, slice<i64> song_ids) -> MixResult<u64> {
    if (! song_ids.is_empty()) {
        auto query =
            rstd_try(mix_query::try_compile_expanded_planned([&](mix_query::QueryBuilder& b) {
                auto insertion = mix_append_rows.insert(b, database_tables, song_ids.len());
                auto rel       = insertion.table();
                return insertion.query()
                    .on_conflict_do_update(
                        array<mix_query::Expr, 2> { rel.col<&RelMixSong::song_id>(),
                                                    rel.col<&RelMixSong::mix_id>() }
                            .as_slice(),
                        array<mix_query::Expr, 2> {
                            rel.col<&RelMixSong::id>().set(b.excluded(rel.col<&RelMixSong::id>())),
                            rel.col<&RelMixSong::update_at>().set(
                                b.excluded(rel.col<&RelMixSong::update_at>())) }
                            .as_slice())
                    .returning(rel.col<&RelMixSong::id>());
            }));
        auto stmt = rstd_try(liorm::sqlito::prepare(tx, rstd::move(query)));
        rstd_try(stmt.bind_rows(mix_append_rows, song_ids.len(), [&](usize index) {
            return tuple { song_ids[index], mix_id, i64() };
        }));
        // Consume all candidates; returned IDs are not final persisted models.
        rstd_try(liorm::sqlito::discard(stmt));
    }
    u64 count;
    {
        auto stmt = rstd_try(liorm::sqlito::prepare(tx, mix_pending_count));
        rstd_try(stmt.bind("mix_id"_str, mix_id));
        auto counted = liorm::sqlito::scalar<Option<i64>>(stmt);
        if (counted.is_err()) {
            auto error = rstd::move(counted).unwrap_err();
            if (error.kind() == sqlito::ErrorKind::Range)
                return Err(MixError { MixErrorReason::Range() });
            return Err(rstd::into<MixError>(rstd::move(error)));
        }
        auto value = rstd::move(counted).unwrap();
        // SeaORM's SQLite paginator decodes COUNT through i32 before u64.
        if (value.is_none() || *value < i64() || *value > i64(2147483647))
            return Err(MixError { MixErrorReason::Range() });
        count = u64(static_cast<unsigned long long>(value->to_primitive()));
    }
    {
        auto stmt = rstd_try(liorm::sqlito::prepare(tx, mix_reorder_append));
        rstd_try(stmt.bind("mix_id"_str, mix_id));
        rstd_try(liorm::sqlito::discard(stmt));
    }
    rstd_try(update_mix_count(tx, mix_id, i64(static_cast<long long>(count.to_primitive()))));
    return Ok(count);
}
class MixStore {
    enum class Operation
    {
        Remove,
        Append
    };
    sqlito::ExecutorHandle handle_;
    static auto execute(sqlito::ExecutorHandle handle, i64 id, Vec<i64> ids, Operation kind)
        -> rstd::async::coro<MixResult<u64>> {
        auto operation =
            handle.submit([id, kind, ids = rstd::move(ids)](
                              sqlito::Connection& db) -> Result<MixResult<u64>, sqlito::Error> {
                auto started = db.begin();
                if (started.is_err())
                    return Ok(MixResult<u64>(
                        Err(rstd::into<MixError>(rstd::move(started).unwrap_err()))));
                auto tx     = rstd::move(started).unwrap();
                auto result = kind == Operation::Append
                                  ? qcm::backend::append_songs(tx, id, ids.as_slice())
                                  : qcm::backend::remove_songs(tx, id, ids.as_slice());
                if (result.is_err()) {
                    auto error = rstd::move(result).unwrap_err();
                    if (tx.rollback().is_err()) error.outcome_unknown = true;
                    return Ok(MixResult<u64>(Err(rstd::move(error))));
                }
                auto committed = tx.commit();
                if (committed.is_err()) {
                    auto error = rstd::into<MixError>(rstd::move(committed).unwrap_err());
                    if (tx.rollback().is_err()) error.outcome_unknown = true;
                    return Ok(MixResult<u64>(Err(rstd::move(error))));
                }
                return Ok(rstd::move(result));
            });
        auto pending = rstd_co_try(rstd::move(operation));
        co_return rstd_co_try(co_await rstd::move(pending), [](sqlito::Error error) {
            auto failure            = rstd::into<MixError>(rstd::move(error));
            failure.outcome_unknown = true;
            return failure;
        });
    }

public:
    explicit MixStore(sqlito::ExecutorHandle handle): handle_(rstd::move(handle)) {}
    auto remove_songs(i64 id, Vec<i64> ids) const -> rstd::async::coro<MixResult<u64>> {
        return execute(handle_.clone(), id, rstd::move(ids), Operation::Remove);
    }
    auto append_songs(i64 id, Vec<i64> ids) const -> rstd::async::coro<MixResult<u64>> {
        return execute(handle_.clone(), id, rstd::move(ids), Operation::Append);
    }
};
} // namespace qcm::backend
