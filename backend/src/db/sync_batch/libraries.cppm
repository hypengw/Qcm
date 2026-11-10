module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:db.sync_batch.libraries;
export import :db.sync_batch.item_reference;
export import :model.library_codec;
export import :db.codec;
import :db.schema;
using rstd::collections::HashSet;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace library_write_query = liorm::query;

export namespace qcm::backend
{
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
LibraryInput {
    String native_id;
    String name;
};
struct LegacyLibraryInput {
    i64                      id;
    String                   name;
    i64                      provider_id;
    String                   native_id;
    Option<DatabaseDateTime> edit_time;
};
} // namespace qcm::backend

namespace qcm::backend
{
inline constexpr library_write_query::ProjectionLayout<library_write_query::ModelGroup<Library>>
                      library_write_projection;
inline constexpr auto library_write_find =
    library_write_query::compile([](library_write_query::QueryBuilder& b) {
        auto library = b.use(database_tables).table<Library>();
        return b.select(library_write_query::project(library_write_projection, library).as_slice())
            .from(library.source())
            .where((library.col<&Library::provider_id>() == b.param<i64>("provider"_str)) &&
                   (library.col<&Library::native_id>() ==
                    b.param<library_write_query::TextParameter>("native"_str)))
            .limit(b.integer(1));
    });
inline constexpr auto library_find_provider =
    library_write_query::compile([](library_write_query::QueryBuilder& b) {
        auto library = b.use(database_tables).table<Library>();
        return b.select(library_write_query::project(library_write_projection, library).as_slice())
            .from(library.source())
            .where(library.col<&Library::provider_id>() == b.param<i64>("provider"_str));
    });
inline constexpr auto library_find_id =
    library_write_query::compile([](library_write_query::QueryBuilder& b) {
        auto library = b.use(database_tables).table<Library>();
        return b.select(library_write_query::project(library_write_projection, library).as_slice())
            .from(library.source())
            .where(library.col<&Library::id>() == b.param<i64>("id"_str));
    });
auto library_write_lookup(sqlito::Transaction& tx, i64 provider, const String& native)
    -> DbWriteResult<Option<Library>> {
    return Ok(rstd_try(liorm::sqlito::fetch_model<library_write_find>(
        tx,
        library_write_projection.group<0>(),
        liorm::parameters(liorm::param<"provider">(provider),
                          liorm::param<"native">(native.as_str())))));
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto read_libraries(sqlito::Connection& db, i64 provider) -> DbWriteResult<Vec<Library>> {
    return Ok(rstd_try(liorm::sqlito::fetch_all<library_find_provider>(
        db,
        liorm::ModelProjection<Library> {},
        liorm::parameters(liorm::param<"provider">(provider)))));
}
auto write_legacy_libraries(sqlito::Transaction& tx, i64 provider,
                            slice<LegacyLibraryInput> entries) -> DbWriteResult<Vec<i64>> {
    using C = liorm::Columns<Library>;
    Vec<i64> ids;
    for (const auto& entry : entries) {
        if (entry.provider_id != provider)
            return Err(DbWriteError::Invalid(String::make("wrong library provider"_str)));
        if (entry.id != i64(-1)) {
            auto existing = rstd_try(liorm::sqlito::fetch_model<library_find_id>(
                tx,
                library_write_projection.group<0>(),
                liorm::parameters(liorm::param<"id">(entry.id)),
                liorm::sqlito::RowConsumption::Drain));
            if (existing.is_none())
                return Err(DbWriteError::Invalid(String::make("missing library"_str)));
            if (existing->provider_id != provider)
                return Err(
                    DbWriteError::Invalid(String::make("library belongs to another provider"_str)));
        }
        auto found = rstd_try(library_write_lookup(tx, provider, entry.native_id));
        auto time  = entry.edit_time.is_some() ? *entry.edit_time : datetime_now();
        if (found.is_some()) {
            liorm::Changeset<Library> changes { C::name = entry.name.clone(), C::edit_time = time };
            auto                      library = rstd_try(
                liorm::sqlito::update<database_tables>(tx, found->id, rstd::move(changes)));
            ids.push(i64(library.id));
        } else {
            liorm::InsertValues<Library> values { C::provider_id = provider,
                                                  C::name        = entry.name.clone(),
                                                  C::native_id   = entry.native_id.clone(),
                                                  C::edit_time   = time };
            if (entry.id != i64(-1)) values.set<&Library::id>(entry.id);
            auto library = rstd_try(liorm::sqlito::insert<database_tables>(tx, rstd::move(values)));
            ids.push(i64(library.id));
        }
    }
    return Ok(rstd::move(ids));
}
// This is only the Library apply stage. The caller owns budget/coordinator
// checks and the transaction, including whether to roll back after an error.
auto write_libraries(sqlito::Transaction& tx, i64 provider, slice<LibraryInput> entries)
    -> DbWriteResult<Vec<Library>> {
    using C = liorm::Columns<Library>;
    HashSet<String> seen;
    Vec<Library>    result;
    for (const auto& entry : entries) {
        auto bytes = entry.native_id.as_str().as_bytes();
        if (bytes.is_empty() || ! seen.insert(entry.native_id.clone()))
            return Err(
                DbWriteError::Invalid(String::make("duplicate or empty library identity"_str)));
        auto found = rstd_try(library_write_lookup(tx, provider, entry.native_id));
        if (found.is_some()) {
            liorm::Changeset<Library> changes { C::name      = entry.name.clone(),
                                                C::edit_time = datetime_now() };
            result.push(rstd_try(
                liorm::sqlito::update<database_tables>(tx, found->id, rstd::move(changes))));
        } else {
            liorm::InsertValues<Library> values { C::provider_id = provider,
                                                  C::name        = entry.name.clone(),
                                                  C::native_id   = entry.native_id.clone(),
                                                  C::edit_time   = datetime_now() };
            result.push(rstd_try(liorm::sqlito::insert<database_tables>(tx, rstd::move(values))));
        }
    }
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
