module;
#include <rstd/macro.hpp>
export module qcm.backend:db.query.media;
export import :error;
import :db.schema;
import :db.error.query;
import :model.item;
import :model.image;
import :model.mix_model;
import :model.song;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace media_query = liorm::query;

namespace qcm::backend::media_detail
{
struct MediaItem {
    i64      provider_id;
    String   native_id;
    ItemType type;
};
struct MediaItemFields {
    Option<i64>    provider_id;
    Option<String> native_id;
    Option<i64>    type;
};
inline constexpr liorm::RecordProjection<MediaItemFields, &MediaItemFields::provider_id,
                                         &MediaItemFields::native_id, &MediaItemFields::type>
                      item_projection;
inline constexpr auto item_query       = media_query::compile([](media_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return item_projection
        .select(b,
                item.col<&Item::provider_id>(),
                item.col<&Item::native_id>(),
                item.col<&Item::type>())
        .from(item.source())
        .where(item.col<&Item::id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});
inline constexpr auto mix_remote_query = media_query::compile([](media_query::QueryBuilder& b) {
    auto mix = b.use(database_tables).table<Mix>();
    return b.select(mix.col<&Mix::remote_id>())
        .from(mix.source())
        .where(mix.col<&Mix::id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});
inline constexpr auto song_album_query = media_query::compile([](media_query::QueryBuilder& b) {
    auto song = b.use(database_tables).table<Song>();
    return b.select(song.col<&Song::album_id>())
        .from(song.source())
        .where(song.col<&Song::id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});
auto                  item_by_id(sqlito::Connection& db, i64 id) -> QueryResult<Option<MediaItem>> {
    auto row =
        rstd_try(liorm::sqlito::fetch_one<item_query>(db,
                                                      item_projection,
                                                      liorm::parameters(liorm::param<"id">(id)),
                                                      liorm::sqlito::RowConsumption::First));
    if (row.is_none()) return Ok(None<MediaItem>());
    return Ok(Some(MediaItem { row->provider_id.unwrap_or(i64()),
                               row->native_id.unwrap_or(String::make()),
                               static_cast<ItemType>(row->type.unwrap_or(i64()).to_primitive()) }));
}
template<const media_query::CompiledQuery& Query>
auto related_id(sqlito::Connection& db, i64 id) -> QueryResult<Option<i64>> {
    auto row = rstd_try((liorm::sqlito::fetch_optional_scalar<Query, Option<i64>>(
        db, liorm::parameters(liorm::param<"id">(id)), liorm::sqlito::RowConsumption::First)));
    return Ok(row.is_some() ? rstd::move(*row) : None<i64>());
}

inline constexpr auto image_native_query = media_query::compile([](media_query::QueryBuilder& b) {
    auto image = b.use(database_tables).table<Image>();
    return b.select(image.col<&Image::native_id>())
        .from(image.source())
        .where((image.col<&Image::item_id>() == b.param<i64>("item_id"_str)) &&
               (image.col<&Image::image_type>() == b.param<i64>("image_type"_str)))
        .limit(b.integer(1));
});

auto image_native(sqlito::Connection& db, i64 owner, ImageType type)
    -> QueryResult<Option<String>> {
    auto row = rstd_try((liorm::sqlito::fetch_optional_scalar<image_native_query, Option<String>>(
        db,
        liorm::parameters(liorm::param<"item_id">(owner),
                          liorm::param<"image_type">(i64(static_cast<int>(type)))),
        liorm::sqlito::RowConsumption::First)));
    return Ok(row.is_some() ? rstd::move(*row) : None<String>());
}
} // namespace qcm::backend::media_detail

export namespace qcm::backend
{
struct AudioResourceDescriptor {
    i64    provider_id;
    String native_id;
};
struct ImageResourceDescriptor {
    i64            provider_id;
    String         native_id;
    Option<String> image_native;
    ImageType      image_type;
};

auto audio_resource_descriptor(sqlito::Connection& db, i64 id)
    -> ProcessResult<Option<AudioResourceDescriptor>> {
    auto item = rstd_try(media_detail::item_by_id(db, id));
    if (item.is_none() || item->type != ItemType::Song) return Ok(None<AudioResourceDescriptor>());
    return Ok(Some(AudioResourceDescriptor { item->provider_id, rstd::move(item->native_id) }));
}

auto image_resource_descriptor(sqlito::Connection& db, i64 id, ref<str> item_name,
                               ref<str> image_name)
    -> ProcessResult<Option<ImageResourceDescriptor>> {
    auto type = rstd::from_str<ItemType>(item_name).ok();
    if (type.is_none() || (*type != ItemType::Album && *type != ItemType::Artist &&
                           *type != ItemType::Mix && *type != ItemType::Song))
        return Err(ProcessError::NoSuchItemType(String::make(item_name)));
    auto image_type = rstd::from_str<ImageType>(image_name).ok();
    if (image_type.is_none()) return Err(ProcessError::NoSuchImageType(String::make(image_name)));
    if (*type == ItemType::Mix) {
        auto remote = rstd_try(media_detail::related_id<media_detail::mix_remote_query>(db, id));
        if (remote.is_some()) id = *remote;
    }
    auto item = rstd_try(media_detail::item_by_id(db, id));
    if (item.is_none() || item->type != *type) return Ok(None<ImageResourceDescriptor>());
    auto native = rstd_try(media_detail::image_native(db, id, *image_type));
    if (native.is_none() && *type == ItemType::Song) {
        auto album = rstd_try(media_detail::related_id<media_detail::song_album_query>(db, id));
        if (album.is_some()) {
            auto owner = rstd_try(media_detail::item_by_id(db, *album));
            if (owner.is_some() && owner->type == ItemType::Album) {
                native          = rstd_try(media_detail::image_native(db, *album, *image_type));
                item->native_id = rstd::move(owner->native_id);
            }
        }
    }
    return Ok(Some(ImageResourceDescriptor {
        item->provider_id, rstd::move(item->native_id), rstd::move(native), *image_type }));
}
} // namespace qcm::backend
