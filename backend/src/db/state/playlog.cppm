export module qcm.backend:db.state.playlog;
export import :error;
export import :playing;
import :db.timestamp;
import sqlito;
using namespace rstd::prelude;

namespace qcm::backend
{
class SyncCoordinator;
struct PlaylogTarget {
    Option<i64> album;
    Option<i64> provider;
    String      native_id;
};
auto read_playlog_target(sqlito::Transaction& tx, i64 song_id) -> ProcessResult<PlaylogTarget>;
} // namespace qcm::backend

export namespace qcm::backend
{
auto record_playlog(sqlito::Connection& db, SyncCoordinator& coordinator,
                    const PlayingState& playing, i64 song_id, Timestamp timestamp,
                    PlayingAction action) -> ProcessResult<empty>;
} // namespace qcm::backend
