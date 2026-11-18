module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :db.sync_batch;
import :db.sync.run;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
namespace
{
auto batch_invalid(ref<str> message) -> DbWriteError {
    return DbWriteError::Invalid(String::make(message));
}
auto batch_add(Vec<i64>& ids, i64 id) -> bool {
    for (const auto& value : ids.as_slice())
        if (value == id) return false;
    ids.push(i64(id));
    return true;
}
} // namespace

auto sync_batch(sqlito::Transaction& tx, i64 provider, const SyncBatch& batch,
                const SyncGuard& guard, usize encoded_bytes) -> DbWriteResult<SyncResult> {
    rstd_try(batch.options.albums.validate<AlbumInput>());
    rstd_try(batch.options.artists.validate<ArtistInput>());
    rstd_try(batch.options.songs.validate<SongInput>());
    rstd_try(batch.options.remote_mixes.validate<RemoteMixInput>());
    rstd_try(batch.options.dynamics.validate<DynamicInput>());

    auto count =
        usize(batch.libraries.len().to_primitive() + batch.albums.len().to_primitive() +
              batch.artists.len().to_primitive() + batch.songs.len().to_primitive() +
              batch.remote_mixes.len().to_primitive() + batch.dynamics.len().to_primitive() +
              batch.images.len().to_primitive() + batch.remote_mix_contents.len().to_primitive());
    if (count > usize(500)) return Err(batch_invalid("batch entity limit exceeded"_str));
    auto refs = usize(batch.song_albums.len().to_primitive() * 2 + 0);
    for (const auto& relation : batch.album_artists.as_slice())
        refs = refs + usize(relation.targets.len().to_primitive() + 1);
    for (const auto& relation : batch.song_artists.as_slice())
        refs = refs + usize(relation.targets.len().to_primitive() + 1);
    if (refs > usize(5000)) return Err(batch_invalid("batch reference limit exceeded"_str));
    auto song_refs = usize(0);
    for (const auto& contents : batch.remote_mix_contents.as_slice())
        song_refs = song_refs + contents.songs.len();
    if (song_refs > usize(10000)) return Err(batch_invalid("batch reference limit exceeded"_str));
    if (encoded_bytes > usize(4 * 1024 * 1024))
        return Err(batch_invalid("batch byte limit exceeded"_str));

    SyncResult result;
    result.libraries = rstd_try(write_libraries(tx, provider, batch.libraries.as_slice()));

    Vec<i64>           ids;
    array<ItemType, 1> album_types { ItemType::Album };
    array<ItemType, 1> artist_types { ItemType::Artist };
    array<ItemType, 1> song_types { ItemType::Song };
    array<ItemType, 2> remote_types { ItemType::Mix, ItemType::Radio };
    for (const auto& entry : batch.albums.as_slice()) {
        auto value = rstd_try(allocate(tx, provider, entry.item, album_types.as_slice()));
        if (! batch_add(ids, value.id)) return Err(batch_invalid("duplicate entity identity"_str));
        result.items.push(rstd::move(value));
    }
    for (const auto& entry : batch.artists.as_slice()) {
        auto value = rstd_try(allocate(tx, provider, entry.item, artist_types.as_slice()));
        if (! batch_add(ids, value.id)) return Err(batch_invalid("duplicate entity identity"_str));
        result.items.push(rstd::move(value));
    }
    for (const auto& entry : batch.songs.as_slice()) {
        auto value = rstd_try(allocate(tx, provider, entry.item, song_types.as_slice()));
        if (! batch_add(ids, value.id)) return Err(batch_invalid("duplicate entity identity"_str));
        result.items.push(rstd::move(value));
    }
    for (const auto& entry : batch.remote_mixes.as_slice()) {
        auto value = rstd_try(allocate(tx, provider, entry.item, remote_types.as_slice()));
        if (! batch_add(ids, value.id)) return Err(batch_invalid("duplicate entity identity"_str));
        result.items.push(rstd::move(value));
    }

    result.albums =
        rstd_try(write_albums(tx, provider, batch.albums.as_slice(), batch.options.albums));
    result.artists =
        rstd_try(write_artists(tx, provider, batch.artists.as_slice(), batch.options.artists));
    auto song_outcome =
        rstd_try(write_songs(tx, provider, batch.songs.as_slice(), batch.options.songs));
    auto new_songs = rstd::move(song_outcome.inserted);
    result.songs   = rstd::move(song_outcome.rows);

    for (const auto& entry : batch.remote_mixes.as_slice()) {
        auto remote = rstd_try(write_remote_mix(tx, provider, entry, batch.options.remote_mixes));
        rstd_try(sync_remote_mix_cache(tx, remote));
        result.remote_mixes.push(rstd::move(remote));
    }

    rstd_try(
        write_dynamics(tx, provider, batch.dynamics.as_slice(), batch.options.dynamics, guard));

    Vec<i64> item_ids;
    for (const auto& item : result.items.as_slice()) item_ids.push(i64(item.id));
    rstd_try(ensure_dynamic_rows(tx, item_ids.as_slice()));

    rstd_try(write_images(tx, provider, batch.images.as_slice()));

    auto allow_album_id = batch.options.songs.allows("album_id"_str);
    rstd_try(write_song_albums(tx,
                               provider,
                               batch.songs.as_slice(),
                               batch.song_albums.as_slice(),
                               allow_album_id,
                               new_songs.as_slice()));
    rstd_try(write_album_artists(tx, provider, batch.album_artists.as_slice()));
    rstd_try(write_song_artists(tx, provider, batch.song_artists.as_slice()));
    rstd_try(write_remote_mix_contents(tx, provider, batch.remote_mix_contents.as_slice()));

    rstd_try(guard.check_origin());
    return Ok(rstd::move(result));
}
} // namespace qcm::backend
