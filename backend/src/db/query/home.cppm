export module qcm.backend:db.query.home;
export import :model.mix;
export import :db.sync_batch.item_reference;
import :db.error.query;
import :model.album;
import :model.artist;
import :model.song;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto home_daily_ids(sqlito::Connection& db, i64 provider) -> QueryResult<Vec<i64>>;
auto home_song(sqlito::Transaction& tx, i64 id) -> QueryResult<Option<Song>>;
auto home_album(sqlito::Transaction& tx, i64 id) -> QueryResult<Option<Album>>;
auto home_artist(sqlito::Transaction& tx, i64 id) -> QueryResult<Option<Artist>>;
auto home_local_mix(sqlito::Transaction& tx, i64 remote_id) -> QueryResult<Option<Mix>>;
} // namespace qcm::backend
