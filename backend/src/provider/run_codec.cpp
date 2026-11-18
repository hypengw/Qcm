module;
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.run_codec;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::lua_run
{
namespace
{
template<class T>
auto decode_legacy_models(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<Vec<T>, luato::Error> {
    return luato::from_value<Vec<T>>(items, options);
}

auto decode_legacy_items(const luato::LuaValue& items, i64 provider,
                         luato::DeserializeOptions options)
    -> Result<Vec<LegacyItemInput>, luato::Error> {
    auto                 models = rstd_try(decode_legacy_models<Item>(items, options));
    Vec<LegacyItemInput> inputs;
    for (auto& model : models) {
        if (model.provider_id != provider)
            return Err(luato::Error::make(luato::ErrorKind::Type,
                                          String::make("qcm.run"_str),
                                          String::make("item belongs to another provider"_str)));
        inputs.push({ rstd::move(model.native_id),
                      model.library_id,
                      model.type,
                      model.create_at,
                      model.update_at,
                      model.last_sync_at });
    }
    return Ok(rstd::move(inputs));
}

auto decode_legacy_libraries(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<Vec<LegacyLibraryInput>, luato::Error> {
    auto                    models = rstd_try(decode_legacy_models<Library>(items, options));
    Vec<LegacyLibraryInput> inputs;
    for (auto& model : models)
        inputs.push({ model.id,
                      rstd::move(model.name),
                      model.provider_id,
                      rstd::move(model.native_id),
                      Some(model.edit_time) });
    return Ok(rstd::move(inputs));
}

auto decode_legacy_images(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<Vec<LegacyImageInput>, luato::Error> {
    auto                  models = rstd_try(decode_legacy_models<Image>(items, options));
    Vec<LegacyImageInput> inputs;
    for (auto& model : models)
        inputs.push({ model.item_id,
                      model.image_type,
                      rstd::move(model.native_id),
                      rstd::move(model.db),
                      rstd::move(model.fresh),
                      Some(model.timestamp) });
    return Ok(rstd::move(inputs));
}

template<class Relation, auto Owner>
auto decode_legacy_relations(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<Vec<LegacyRelationInput>, luato::Error> {
    auto                     models = rstd_try(decode_legacy_models<Relation>(items, options));
    Vec<LegacyRelationInput> inputs;
    for (const auto& model : models)
        inputs.push({ model.*Owner, model.artist_id, model.update_at });
    return Ok(rstd::move(inputs));
}
} // namespace

auto sync_libraries(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(decode_legacy_libraries(items, options));
    return Ok(LuaRunCommand::SyncLibraries(rstd::move(inputs)));
}

auto sync_songs(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(luato::from_value<Vec<Song>>(items, options));
    return Ok(LuaRunCommand::SyncSongs(rstd::move(inputs)));
}

auto sync_images(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(decode_legacy_images(items, options));
    return Ok(LuaRunCommand::SyncImages(rstd::move(inputs)));
}

auto sync_artists(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(luato::from_value<Vec<Artist>>(items, options));
    return Ok(LuaRunCommand::SyncArtists(rstd::move(inputs)));
}

auto sync_radio_queues(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(decode_legacy_models<RadioQueueInput>(items, options));
    return Ok(LuaRunCommand::SyncRadioQueues(rstd::move(inputs)));
}

auto sync_albums(const luato::LuaValue& items, LegacySyncOptions sync_options,
                 luato::DeserializeOptions options) -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(luato::from_value<Vec<Album>>(items, options));
    return Ok(LuaRunCommand::SyncAlbums(rstd::move(inputs), rstd::move(sync_options)));
}

auto sync_dynamics(const luato::LuaValue& items, LegacySyncOptions sync_options,
                   luato::DeserializeOptions options) -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(decode_legacy_models<Dynamic>(items, options));
    return Ok(LuaRunCommand::SyncDynamics(rstd::move(inputs), rstd::move(sync_options)));
}

auto sync_remote_mixes(const luato::LuaValue& items, LegacySyncOptions sync_options,
                       luato::DeserializeOptions options) -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(decode_legacy_models<RemoteMix>(items, options));
    return Ok(LuaRunCommand::SyncRemoteMixes(rstd::move(inputs), rstd::move(sync_options)));
}

auto sync_album_artist_ids(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(
        (decode_legacy_relations<RelAlbumArtist, &RelAlbumArtist::album_id>(items, options)));
    return Ok(LuaRunCommand::SyncAlbumArtistIds(rstd::move(inputs)));
}

auto sync_song_artist_ids(const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs =
        rstd_try((decode_legacy_relations<RelSongArtist, &RelSongArtist::song_id>(items, options)));
    return Ok(LuaRunCommand::SyncSongArtistIds(rstd::move(inputs)));
}

auto allocate_items(const luato::LuaValue& items, i64 provider, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto inputs = rstd_try(decode_legacy_items(items, provider, options));
    return Ok(LuaRunCommand::AllocateItems(rstd::move(inputs)));
}

auto sync_remote_mix_song_ids(i64 mix, const luato::LuaValue& items,
                              luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto ids = rstd_try(luato::from_value<Vec<i64>>(items, options));
    return Ok(LuaRunCommand::SyncRemoteMixSongIds(mix, rstd::move(ids)));
}

auto observe_items(ObserveScope scope, const luato::LuaValue& items,
                   luato::DeserializeOptions options) -> Result<LuaRunCommand, luato::Error> {
    auto keys = rstd_try(luato::from_value<Vec<ItemRef>>(items, options));
    return Ok(LuaRunCommand::Observe(rstd::move(scope), rstd::move(keys)));
}

auto resolve_items(ref<str> name, const luato::LuaValue& items, luato::DeserializeOptions options)
    -> Result<LuaRunCommand, luato::Error> {
    auto kind = rstd::from_str<ItemType>(name).ok();
    if (kind.is_none())
        return Err(luato::Error::make(luato::ErrorKind::Type,
                                      String::make("qcm.run"_str),
                                      String::make("invalid resolve_items arguments"_str)));
    auto ids = rstd_try(luato::from_value<Vec<String>>(items, options));
    if (ids.len() > usize(500)) return Err(run_error("reference limit exceeded"_str));
    return Ok(LuaRunCommand::Resolve(*kind, rstd::move(ids)));
}
} // namespace qcm::backend::lua_run
