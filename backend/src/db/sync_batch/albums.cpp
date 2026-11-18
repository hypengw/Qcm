module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :db.sync_batch;
import liorm.sqlito;
import :db.codec;
import :db.schema;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace album_query = liorm::query;

namespace qcm::backend
{
namespace
{
auto find_album(sqlito::Transaction& tx, i64 id) -> DbWriteResult<Option<Album>> {
    return Ok(rstd_try((liorm::sqlito::find<database_tables, Album>(tx, id))));
}
} // namespace

auto write_legacy_albums(sqlito::Transaction& tx, i64 provider, slice<Album> entries,
                         const Mask& mask) -> DbWriteResult<Vec<i64>> {
    rstd_try(mask.validate<AlbumInput>());
    const auto&        selected = mask;
    array<ItemType, 1> expected { ItemType::Album };
    Vec<i64>           ids;
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.id), expected.as_slice()));
        auto old = rstd_try(find_album(tx, entry.id));
        if (old.is_none()) {
            Album model { entry.id,
                          entry.name.clone(),
                          entry.sort_name.clone(),
                          entry.track_count,
                          entry.disc_count,
                          entry.type,
                          entry.duration,
                          entry.language.clone(),
                          entry.description.clone(),
                          entry.company.clone(),
                          entry.publish_time,
                          entry.added_at };
            rstd_try(liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model)));
        } else {
            liorm::Changeset<Album> changes;
            sync_schema::Definition<AlbumFields>::apply_changes(changes, entry, selected);
            if (! changes.empty())
                rstd_try(liorm::sqlito::update<database_tables>(tx, entry.id, rstd::move(changes)));
        }
        ids.push(i64(entry.id));
    }
    return Ok(rstd::move(ids));
}

auto write_albums(sqlito::Transaction& tx, i64 provider, slice<AlbumInput> entries,
                  const Mask& mask) -> DbWriteResult<Vec<Album>> {
    array<ItemType, 1> expected { ItemType::Album };
    Vec<Album>         result;
    for (const auto& entry : entries) {
        auto item     = rstd_try(resolve(tx, provider, entry.item, expected.as_slice()));
        auto old      = rstd_try(find_album(tx, item.id));
        auto inserted = old.is_none();
        auto model =
            inserted ? Album { item.id, String::make(), None(), i32(),  i32(),  AlbumType::Album,
                               i64(),   None(),         None(), None(), None(), None() }
                     : rstd::move(old).unwrap();
        rstd_try(entry.apply(model, inserted, mask));
        if (model.name.is_empty())
            return Err(DbWriteError::Invalid(String::make("entity name is required"_str)));
        auto written =
            rstd_try(inserted ? liorm::sqlito::insert_all<database_tables>(tx, rstd::move(model))
                              : liorm::sqlito::update_all<database_tables>(tx, rstd::move(model)));
        result.push(rstd::move(written));
    }
    for (const auto& album : result.as_slice())
        if (album.duration < i64() || album.track_count < i32() || album.disc_count < i32())
            return Err(DbWriteError::Invalid(String::make("negative album quantity"_str)));
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
