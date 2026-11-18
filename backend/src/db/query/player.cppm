export module qcm.backend:db.query.player;
import :error;
import sqlito;
import rstd;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct PlayerTrack {
    String      title;
    String      album;
    Vec<String> artists;
    Vec<String> album_artists;
    i64         duration {};
    bool        can_play {};
};
auto player_track(sqlito::Connection& db, i64 id) -> ProcessResult<PlayerTrack>;
} // namespace qcm::backend
