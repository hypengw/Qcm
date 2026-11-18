module;
#include <rstd/enum.hpp>
export module qcm.backend:model.mix;
export import liorm;
export import sqlito;
export import :model.rel_mix_song;
import :db.error.query;
import :model.mix_model;
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
struct MixSongPage {
    Vec<i64> ids;
    i64      total {};
};
template<class Database>
auto mix_song_ids(Database& db, i64 mix_id, i32 page, i32 page_size, MixSongOrder order, bool asc)
    -> QueryResult<MixSongPage>;
template<class Database>
auto mix_find(Database& db, i64 id) -> QueryResult<Option<i64>>;

extern template auto mix_song_ids(sqlito::Connection&, i64, i32, i32, MixSongOrder, bool)
    -> QueryResult<MixSongPage>;
extern template auto mix_song_ids(sqlito::Transaction&, i64, i32, i32, MixSongOrder, bool)
    -> QueryResult<MixSongPage>;
extern template auto mix_find(sqlito::Connection&, i64) -> QueryResult<Option<i64>>;
extern template auto mix_find(sqlito::Transaction&, i64) -> QueryResult<Option<i64>>;
} // namespace qcm::backend
export namespace qcm::backend
{
// The caller owns the transaction and must roll it back after an error.
auto remove_songs(sqlito::Transaction& tx, i64 mix_id, slice<i64> song_ids) -> MixResult<u64>;
// Empty input skips only the INSERT: existing zero-order rows still get appended.
// As with remove_songs, the caller owns rollback/commit of this transaction.
auto append_songs(sqlito::Transaction& tx, i64 mix_id, slice<i64> song_ids) -> MixResult<u64>;
class MixStore {
    enum class Operation
    {
        Remove,
        Append
    };
    sqlito::ExecutorHandle handle_;
    static auto execute(sqlito::ExecutorHandle handle, i64 id, Vec<i64> ids, Operation kind)
        -> rstd::async::coro<MixResult<u64>>;

public:
    explicit MixStore(sqlito::ExecutorHandle handle);
    auto remove_songs(i64 id, Vec<i64> ids) const -> rstd::async::coro<MixResult<u64>>;
    auto append_songs(i64 id, Vec<i64> ids) const -> rstd::async::coro<MixResult<u64>>;
};
} // namespace qcm::backend
