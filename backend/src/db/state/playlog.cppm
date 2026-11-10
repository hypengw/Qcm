module;
#include <rstd/macro.hpp>
export module qcm.backend:db.state.playlog;
export import :error;
export import :playing;
import :db.sync_batch;
import :db.sync.coordination;
import :db.error.process;
import :db.schema;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace playlog_query = liorm::query;

namespace qcm::backend
{
struct PlaylogTarget {
    Option<i64> album;
    Option<i64> provider;
    String      native_id;
};
inline constexpr liorm::RecordProjection<PlaylogTarget, &PlaylogTarget::album,
                                         &PlaylogTarget::provider, &PlaylogTarget::native_id>
                      playlog_projection;
inline constexpr auto playlog_target = playlog_query::compile([](playlog_query::QueryBuilder& b) {
    auto schema = b.use(database_tables);
    auto song   = schema.table<Song>();
    auto item   = schema.table<Item>();
    return playlog_projection
        .select(b,
                song.col<&Song::album_id>(),
                item.col<&Item::provider_id>(),
                item.col<&Item::native_id>())
        .from(song.source())
        .left_join(item.source(), song.col<&Song::id>() == item.col<&Item::id>())
        .where(song.col<&Song::id>() == b.param<i64>("id"_str));
});
auto read_playlog_target(sqlito::Transaction& tx, i64 song_id) -> ProcessResult<PlaylogTarget> {
    auto query = rstd_try(liorm::sqlito::prepare(tx, playlog_target));
    rstd_try(query.bind("id"_str, song_id));
    if (rstd_try(query.step()) == sqlito::Step::Done)
        return Err(ProcessError::NoSuchSong(rstd::format("{}", song_id)));
    auto          reader = query.reader();
    PlaylogTarget target;
    target.album    = rstd_try(playlog_projection.field<&PlaylogTarget::album>(reader));
    target.provider = rstd_try(playlog_projection.field<&PlaylogTarget::provider>(reader));
    if (target.provider.is_some())
        target.native_id = rstd_try(playlog_projection.field<&PlaylogTarget::native_id>(reader));
    return Ok(rstd::move(target));
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto record_playlog(sqlito::Connection& db, SyncCoordinator& coordinator,
                    const PlayingState& playing, i64 song_id, Timestamp timestamp,
                    PlayingAction action) -> ProcessResult<empty> {
    array<i64, 1> ids { song_id };
    auto          guards = rstd_try(write_items(coordinator, db, ids.as_slice()));
    coordinator.bump();
    auto tx     = rstd_try(db.begin());
    auto target = rstd_try(read_playlog_target(tx, song_id));
    if (target.provider.is_some()) {
        if (action == PlayingAction::Play)
            playing.set(*target.provider, PlayingInfo { song_id, rstd::move(target.native_id) });
        else if (action == PlayingAction::Stop)
            playing.clear(*target.provider);
    }
    rstd_try(ensure_dynamic_rows(tx, ids.as_slice()));
    rstd_try(record_play(tx, song_id, target.album, timestamp));
    rstd_try(tx.commit());
    return Ok(empty {});
}
} // namespace qcm::backend
