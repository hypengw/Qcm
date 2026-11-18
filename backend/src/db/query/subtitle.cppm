export module qcm.backend:db.query.subtitle;
import :db.error.query;
import sqlito;
import rstd;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct SubtitleTarget {
    i64    provider_id;
    String native_id;
};
auto subtitle_target(sqlito::Connection& db, i64 id) -> QueryResult<Option<SubtitleTarget>>;
} // namespace qcm::backend
