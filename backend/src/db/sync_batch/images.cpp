module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :db.sync_batch;
import liorm.sqlito;
import :db.schema;
import :db.timestamp;
import :db.codec;
using rstd::tuple;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace image_query = liorm::query;

namespace qcm::backend
{
namespace
{
inline constexpr image_query::ProjectionLayout<image_query::ModelGroup<Image>> image_projection;
inline constexpr auto image_find   = image_query::compile([](image_query::QueryBuilder& b) {
    auto table = b.use(database_tables).table<Image>();
    return b.select(image_query::project(image_projection, table).as_slice())
        .from(table.source())
        .where((table.col<&Image::item_id>() == b.param<i64>("item_id"_str)) &&
               (table.col<&Image::image_type>() == b.param<i64>("image_type"_str)))
        .limit(b.integer(1));
});
inline constexpr auto image_remove = image_query::compile([](image_query::QueryBuilder& b) {
    auto table = b.use(database_tables).table<Image>();
    return b.delete_from(table.source()).where(table.col<&Image::id>() == b.param<i64>("id"_str));
});
auto                  find_image(sqlito::Transaction& tx, i64 item_id, ImageType type)
    -> DbWriteResult<Option<Image>> {
    return Ok(rstd_try(liorm::sqlito::fetch_model<image_find>(
        tx,
        image_projection.group<0>(),
        liorm::parameters(liorm::param<"item_id">(item_id),
                          liorm::param<"image_type">(i64(static_cast<int>(type)))))));
}
auto delete_image(sqlito::Transaction& tx, i64 id) -> DbWriteResult<empty> {
    auto stmt = rstd_try(
        liorm::sqlito::prepare<image_remove>(tx, liorm::parameters(liorm::param<"id">(id))));
    rstd_try(liorm::sqlito::discard(stmt));
    return Ok(empty {});
}
auto image_contains(const Vec<tuple<i64, ImageType>>& seen, i64 id, ImageType type) -> bool {
    for (const auto& entry : seen.as_slice())
        if (rstd::get<0>(entry) == id && rstd::get<1>(entry) == type) return true;
    return false;
}
auto image_insert(sqlito::Transaction& tx, Image model) -> DbWriteResult<empty> {
    using C = liorm::Columns<Image>;
    liorm::InsertValues<Image> values { C::item_id    = model.item_id,
                                        C::image_type = model.image_type,
                                        C::native_id  = rstd::move(model.native_id),
                                        C::db         = rstd::move(model.db),
                                        C::fresh      = rstd::move(model.fresh),
                                        C::timestamp  = rstd::move(model.timestamp) };
    rstd_try(liorm::sqlito::insert<database_tables>(tx, rstd::move(values)));
    return Ok(empty {});
}
} // namespace

auto write_legacy_images(sqlito::Transaction& tx, i64 provider, slice<LegacyImageInput> entries)
    -> DbWriteResult<empty> {
    using C = liorm::Columns<Image>;
    for (const auto& entry : entries) {
        rstd_try(resolve(tx, provider, ItemRef::Id(entry.item_id), slice<ItemType> {}));
        auto old    = rstd_try(find_image(tx, entry.item_id, entry.image_type));
        auto millis = current_millis().to_primitive();
        auto time   = entry.timestamp.is_some()
                          ? *entry.timestamp
                          : DatabaseDateTime { i64(millis / 1000),
                                               static_cast<unsigned>(millis % 1000 * 1000000) };
        auto native =
            entry.native_id.is_some() ? Some(entry.native_id->clone()) : Option<String>(None());
        auto db = entry.db.is_some() ? Some(entry.db->clone()) : Option<String>(None());
        if (old.is_none()) {
            Image model { i64(),
                          entry.item_id,
                          entry.image_type,
                          rstd::move(native),
                          rstd::move(db),
                          entry.fresh.clone(),
                          time };
            rstd_try(image_insert(tx, rstd::move(model)));
        } else {
            liorm::Changeset<Image> changes { C::native_id = rstd::move(native),
                                              C::db        = rstd::move(db),
                                              C::fresh     = entry.fresh.clone(),
                                              C::timestamp = time };
            rstd_try(liorm::sqlito::update<database_tables>(tx, old->id, rstd::move(changes)));
        }
    }
    return Ok(empty {});
}

auto write_images(sqlito::Transaction& tx, i64 provider, slice<ImageInput> entries)
    -> DbWriteResult<empty> {
    Vec<tuple<i64, ImageType>> seen;
    for (const auto& entry : entries) {
        auto item = rstd_try(resolve(tx, provider, entry.item_id, slice<ItemType> {}));
        if (image_contains(seen, item.id, entry.image_type))
            return Err(DbWriteError::Invalid(String::make("duplicate image identity"_str)));
        seen.push(tuple<i64, ImageType> { i64(item.id), entry.image_type });
        auto old = rstd_try(find_image(tx, item.id, entry.image_type));
        if (entry.remove) {
            if (old.is_some()) rstd_try(delete_image(tx, old->id));
            continue;
        }
        auto millis = current_millis().to_primitive();
        auto now =
            DatabaseDateTime { i64(millis / 1000), static_cast<unsigned>(millis % 1000 * 1000000) };
        if (old.is_none()) {
            Option<String> native = Option<String>(None());
            if (entry.native_id.is_set() && entry.native_id.value().is_some())
                native = entry.native_id.value().clone();
            String fresh;
            if (entry.fresh.is_set()) fresh = entry.fresh.value().clone();
            Image model { i64(),
                          i64(item.id),
                          entry.image_type,
                          rstd::move(native),
                          Option<String>(None()),
                          rstd::move(fresh),
                          now };
            rstd_try(image_insert(tx, rstd::move(model)));
        } else {
            liorm::Changeset<Image> changes;
            if (entry.native_id.is_set()) {
                const auto& value = entry.native_id.value();
                changes.set<&Image::native_id>(value.is_some() ? Some(value->clone())
                                                               : Option<String>(None()));
            }
            if (entry.fresh.is_set()) changes.set<&Image::fresh>(entry.fresh.value().clone());
            changes.set<&Image::timestamp>(now);
            rstd_try(liorm::sqlito::update<database_tables>(tx, old->id, rstd::move(changes)));
        }
    }
    return Ok(empty {});
}
} // namespace qcm::backend
