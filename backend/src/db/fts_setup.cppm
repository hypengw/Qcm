module;
#include <rstd/macro.hpp>
export module qcm.backend:db.fts_setup;
export import :fts;
export import rstd;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
// Keeps the qcm tokenizer and qcm_query scalar registered for one connection.
struct QcmFts {
    sqlito::FtsRegistration    tokenizer;
    sqlito::ScalarRegistration query;
};
auto register_qcm_fts(sqlito::Connection& db) -> Result<QcmFts, sqlito::Error> {
    auto tokenizer =
        rstd_try(db.register_or_replace_fts5_tokenizer(String::make("qcm"_str), qcm_fts_factory()));
    auto query = rstd_try(db.register_or_replace_scalar_function(
        String::make("qcm_query"_str), qcm_fts_scalar(), true));
    return Ok(QcmFts { rstd::move(tokenizer), rstd::move(query) });
}
} // namespace qcm::backend
