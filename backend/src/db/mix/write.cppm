module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.mix.write;
export import :error;
export import :db.sync.coordination;
import :db.schema;
import :db.timestamp;
import :model.mix;
import :model.mix_model;
import :model.song;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace mix_write_query = liorm::query;

export namespace qcm::backend
{
class MixContentChange {
    RSTD_ENUM(MixContentChange, (AppendSongs, (Vec<i64> ids;)), (RemoveSongs, (Vec<i64> ids;)),
              (AppendAlbums, (Vec<i64> ids;)))
};
struct MixContentWrite {
    i64              mix_id;
    MixContentChange change;
};
} // namespace qcm::backend

namespace qcm::backend
{
auto mix_process_error(MixError error) -> ProcessError {
    RSTD_MATCH(error.reason) {
        RSTD_CASE(Storage, cause) { return ProcessError::db(rstd::move(cause)); }
        RSTD_CASE_DEFAULT { return ProcessError::internal(rstd::move(error)); }
    }
}
auto mix_write_time() -> Timestamp {
    return Timestamp::from_millis(Timestamp::now().as_millis() / i64(1000) * i64(1000));
}
inline constexpr auto mix_create_query =
    mix_write_query::compile([](mix_write_query::QueryBuilder& b) {
        auto mix = b.use(database_tables).table<Mix>();
        return b
            .insert_into(mix.source(),
                         mix.col<&Mix::name>(),
                         mix.col<&Mix::track_count>(),
                         mix.col<&Mix::description>(),
                         mix.col<&Mix::mix_type>(),
                         mix.col<&Mix::content_update_at>(),
                         mix.col<&Mix::create_at>(),
                         mix.col<&Mix::update_at>())
            .values(b.param<mix_write_query::TextParameter>("name"_str),
                    b.integer(0),
                    b.text_value(""_str),
                    b.integer(0),
                    b.integer(0),
                    b.param<Timestamp>("now"_str),
                    b.param<Timestamp>("now"_str))
            .returning(mix.col<&Mix::id>());
    });
template<class Database>
auto mix_change_types(Database& db, slice<i64> ids, bool remove) -> ProcessResult<empty> {
    namespace q = mix_write_query;
    q::QueryBuilder b;
    auto            mix        = b.use(database_tables).table<Mix>();
    auto            parameters = b.param_group<i64>("ids"_str, ids.len());
    if (remove) {
        auto query = rstd_try(
            q::try_compile_expanded(b.delete_from(mix.source())
                                        .where((mix.col<&Mix::mix_type>() == b.integer(0)) &&
                                               mix.col<&Mix::id>().in(parameters))),
            ProcessError::internal(rstd::move(_e)));
        auto stmt = rstd_try(liorm::sqlito::prepare(db, rstd::move(query)));
        rstd_try(stmt.bind_group("ids"_str, ids));
        rstd_try(liorm::sqlito::discard(stmt));
    }
    auto condition = mix.col<&Mix::id>().in(parameters);
    if (remove) condition = condition && (mix.col<&Mix::mix_type>() == b.integer(1));
    auto query =
        rstd_try(q::try_compile_expanded(
                     b.update(mix.source())
                         .set(mix.col<&Mix::mix_type>().set(b.integer(remove ? 2 : 1)),
                              mix.col<&Mix::update_at>().set(b.param<Timestamp>("now"_str)))
                         .where(condition)),
                 ProcessError::internal(rstd::move(_e)));
    auto stmt = rstd_try(liorm::sqlito::prepare(db, rstd::move(query)));
    rstd_try(stmt.bind_group("ids"_str, ids));
    rstd_try(stmt.bind("now"_str, mix_write_time()));
    rstd_try(liorm::sqlito::discard(stmt));
    return Ok(empty {});
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto create_mix_record(sqlito::Connection& db, SyncCoordinator& coordinator, String name)
    -> ProcessResult<i64> {
    auto guard = rstd_try(coordinator.write(i64()));
    coordinator.bump();
    auto tx   = rstd_try(db.begin());
    auto stmt = rstd_try(liorm::sqlito::prepare(tx, mix_create_query));
    rstd_try(stmt.bind("name"_str, name));
    rstd_try(stmt.bind("now"_str, mix_write_time()));
    if (rstd_try(stmt.step()) != sqlito::Step::Row)
        return Err(ProcessError::internal("missing mix insert id"_str));
    auto id = rstd_try(stmt.reader().read<i64>(usize()));
    rstd_try(liorm::sqlito::discard(stmt));
    rstd_try(tx.commit());
    return Ok(id);
}
auto delete_mix_records(sqlito::Connection& db, SyncCoordinator& coordinator, Vec<i64> ids)
    -> ProcessResult<empty> {
    auto guards = rstd_try(write_mixes(coordinator, db, ids.as_slice(), slice<i64> {}));
    coordinator.bump();
    if (ids.is_empty()) return Ok(empty {});
    auto tx = rstd_try(db.begin());
    rstd_try(mix_change_types(tx, ids.as_slice(), true));
    rstd_try(tx.commit());
    return Ok(empty {});
}
auto link_mix_records(sqlito::Connection& db, SyncCoordinator& coordinator, Vec<i64> ids)
    -> ProcessResult<empty> {
    auto guards = rstd_try(write_mixes(coordinator, db, ids.as_slice(), slice<i64> {}));
    coordinator.bump();
    if (ids.is_empty()) return Ok(empty {});
    auto tx = rstd_try(db.begin());
    rstd_try(mix_change_types(tx, ids.as_slice(), false));
    rstd_try(tx.commit());
    return Ok(empty {});
}
auto write_mix_content(sqlito::Connection& db, SyncCoordinator& coordinator, MixContentWrite input)
    -> ProcessResult<u64> {
    Vec<i64> affected;
    RSTD_MATCH(input.change) {
        RSTD_CASE(AppendSongs, ids) {
            for (auto id : ids) affected.push(i64(id));
        }
        RSTD_CASE(RemoveSongs, ids) {
            for (auto id : ids) affected.push(i64(id));
        }
        RSTD_CASE(AppendAlbums, ids) {
            for (auto id : ids) affected.push(i64(id));
        }
    }
    array<i64, 1> mix_ids { input.mix_id };
    auto guards = rstd_try(write_mixes(coordinator, db, mix_ids.as_slice(), affected.as_slice()));
    coordinator.bump();
    auto tx = rstd_try(db.begin());
    u64  count {};
    RSTD_MATCH(input.change) {
        RSTD_CASE(AppendSongs, ids) {
            count = rstd_try(append_songs(tx, input.mix_id, ids.as_slice()),
                             mix_process_error(rstd::move(_e)));
        }
        RSTD_CASE(RemoveSongs, ids) {
            count = rstd_try(remove_songs(tx, input.mix_id, ids.as_slice()),
                             mix_process_error(rstd::move(_e)));
        }
        RSTD_CASE(AppendAlbums, ids) {
            Vec<i64> songs;
            if (! ids.is_empty()) {
                namespace q = mix_write_query;
                q::QueryBuilder b;
                auto            song  = b.use(database_tables).table<Song>();
                auto            query = rstd_try(
                    q::try_compile_expanded(b.select(song.col<&Song::id>())
                                                .from(song.source())
                                                .where(song.col<&Song::album_id>().in(
                                                    b.param_group<i64>("albums"_str, ids.len())))),
                    ProcessError::internal(rstd::move(_e)));
                auto stmt = rstd_try(liorm::sqlito::prepare(tx, rstd::move(query)));
                rstd_try(stmt.bind_group("albums"_str, ids.as_slice()));
                while (rstd_try(stmt.step()) == sqlito::Step::Row)
                    songs.push(rstd_try(stmt.reader().read<i64>(usize())));
            }
            count = rstd_try(append_songs(tx, input.mix_id, songs.as_slice()),
                             mix_process_error(rstd::move(_e)));
        }
    }
    rstd_try(tx.commit());
    return Ok(count);
}
} // namespace qcm::backend
