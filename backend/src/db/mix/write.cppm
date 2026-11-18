module;
#include <rstd/enum.hpp>
export module qcm.backend:db.mix.write;
export import :error;
export import :db.sync.run;
import sqlito;
using namespace rstd::prelude;

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

auto create_mix_record(sqlito::Connection& db, SyncCoordinator& coordinator, String name)
    -> ProcessResult<i64>;
auto delete_mix_records(sqlito::Connection& db, SyncCoordinator& coordinator, Vec<i64> ids)
    -> ProcessResult<empty>;
auto link_mix_records(sqlito::Connection& db, SyncCoordinator& coordinator, Vec<i64> ids)
    -> ProcessResult<empty>;
auto write_mix_content(sqlito::Connection& db, SyncCoordinator& coordinator, MixContentWrite input)
    -> ProcessResult<u64>;
} // namespace qcm::backend
