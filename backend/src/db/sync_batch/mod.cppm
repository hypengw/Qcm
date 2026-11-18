export module qcm.backend:db.sync_batch;
export import :db.sync_batch.item_reference;
export import :model.album;
export import :model.artist;
export import :model.dynamic;
export import :model.image;
export import :model.library;
export import :model.mix_model;
export import :model.remote_mix;
export import :model.song;
using namespace rstd::prelude;

namespace qcm::backend
{
class SyncGuard;
} // namespace qcm::backend

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

auto read_libraries(sqlito::Connection& db, i64 provider) -> DbWriteResult<Vec<Library>>;
auto write_legacy_libraries(sqlito::Transaction& tx, i64 provider,
                            slice<LegacyLibraryInput> entries) -> DbWriteResult<Vec<i64>>;
// This is only the Library apply stage. The caller owns budget/coordinator
// checks and the transaction, including whether to roll back after an error.
auto write_libraries(sqlito::Transaction& tx, i64 provider, slice<LibraryInput> entries)
    -> DbWriteResult<Vec<Library>>;

[[pmacro::invoke("qcm::entity_input", AlbumInput, AlbumFields, Album, {
    name: String,
    sort_name: Option<String>,
    track_count: i32,
    disc_count: i32,
    type: AlbumType,
    duration: i64,
    language: Option<String>,
    description: Option<String>,
    company: Option<String>,
    publish_time: Option<Timestamp>,
    added_at: Option<Timestamp>,
})]];

auto write_legacy_albums(sqlito::Transaction& tx, i64 provider, slice<Album> entries,
                         const Mask& mask) -> DbWriteResult<Vec<i64>>;
// The caller owns the transaction and the mask prevalidation for empty groups.
auto write_albums(sqlito::Transaction& tx, i64 provider, slice<AlbumInput> entries,
                  const Mask& mask) -> DbWriteResult<Vec<Album>>;

[[pmacro::invoke("qcm::entity_input", ArtistInput, ArtistFields, Artist, {
    name: String,
    sort_name: Option<String>,
    description: String,
    album_count: i32,
    music_count: i32,
    added_at: Option<Timestamp>,
})]];

auto write_legacy_artists(sqlito::Transaction& tx, i64 provider, slice<Artist> entries)
    -> DbWriteResult<Vec<i64>>;
// The caller owns the transaction and the mask prevalidation for empty groups.
auto write_artists(sqlito::Transaction& tx, i64 provider, slice<ArtistInput> entries,
                   const Mask& mask) -> DbWriteResult<Vec<Artist>>;

struct SongTags {
    static auto convert(const Vec<String>& value) -> rstd::json::Value;
};
[[pmacro::invoke("qcm::entity_input", SongInput, SongFields, Song, {
    name: String,
    sort_name: Option<String>,
    track_number: i32,
    disc_number: i32,
    duration: i64,
    can_play: bool,
    popularity: f64,
    tags: Vec<String>[convert(SongTags)],
    publish_time: Option<Timestamp>,
    added_at: Option<Timestamp>,
},
                 {
                     album_id: Patch<Option<ItemRef>>,
                 })]];
struct SongWriteOutcome {
    Vec<Song> rows;
    Vec<i64>  inserted;
};

auto write_legacy_songs(sqlito::Transaction& tx, i64 provider, slice<Song> entries)
    -> DbWriteResult<Vec<i64>>;
// The caller owns the transaction and the mask prevalidation for empty groups.
auto write_songs(sqlito::Transaction& tx, i64 provider, slice<SongInput> entries, const Mask& mask)
    -> DbWriteResult<SongWriteOutcome>;

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
SongAlbumInput {
    ItemRef song;
    [[pmacro::helper("serde::derive", default)]]
    Option<ItemRef> album;
};
struct LegacySongAlbumInput {
    String song_native_id;
    String album_native_id;
};

auto write_legacy_song_album_ids(sqlito::Transaction& tx, i64 provider, i64 library,
                                 slice<LegacySongAlbumInput> entries)
    -> Result<empty, DbWriteError>;
// The caller owns the transaction and the song rows written earlier in the batch.
auto write_song_albums(sqlito::Transaction& tx, i64 provider, slice<SongInput> songs,
                       slice<SongAlbumInput> relations, bool songs_allow_album_id,
                       slice<i64> new_songs) -> DbWriteResult<empty>;

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
ImageInput {
    ItemRef   item_id;
    ImageType image_type;
    [[pmacro::helper("serde::derive", default)]]
    Patch<Option<String>> native_id;
    [[pmacro::helper("serde::derive", default)]]
    Patch<String> fresh;
    [[pmacro::helper("serde::derive", default)]]
    bool remove {};
};
struct LegacyImageInput {
    i64                      item_id;
    ImageType                image_type;
    Option<String>           native_id;
    Option<String>           db;
    String                   fresh;
    Option<DatabaseDateTime> timestamp;
};

auto write_legacy_images(sqlito::Transaction& tx, i64 provider, slice<LegacyImageInput> entries)
    -> DbWriteResult<empty>;
// The caller owns the transaction; item resolution and duplicate checks happen before writes.
auto write_images(sqlito::Transaction& tx, i64 provider, slice<ImageInput> entries)
    -> DbWriteResult<empty>;

enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", decode_only)]] RelationMode
{
    Merge [[pmacro::helper("serde::derive", rename("merge"))]],
    Replace [[pmacro::helper("serde::derive", rename("replace"))]],
};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
RelationInput {
    ItemRef      owner;
    RelationMode mode;
    Vec<ItemRef> targets;
};
struct LegacyRelationInput {
    i64       owner_id;
    i64       artist_id;
    Timestamp update_at;
};

auto write_legacy_album_artists(sqlito::Transaction& tx, i64 provider,
                                slice<LegacyRelationInput> entries) -> DbWriteResult<empty>;
auto write_legacy_song_artists(sqlito::Transaction& tx, i64 provider,
                               slice<LegacyRelationInput> entries) -> DbWriteResult<empty>;
// The caller owns the transaction and runs relations after their owners exist.
auto write_album_artists(sqlito::Transaction& tx, i64 provider, slice<RelationInput> entries)
    -> DbWriteResult<empty>;
auto write_song_artists(sqlito::Transaction& tx, i64 provider, slice<RelationInput> entries)
    -> DbWriteResult<empty>;

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
MixContents {
    ItemRef      mix;
    RelationMode mode;
    Vec<ItemRef> songs;
};

auto write_legacy_mix_songs(sqlito::Transaction& tx, i64 provider, i64 remote_id, slice<i64> songs)
    -> DbWriteResult<empty>;
// This is the ordered-contents apply stage, not the outer SyncBatch owner:
// budget/generation checks and commit/rollback belong to the caller.
auto write_remote_mix_contents(sqlito::Transaction& tx, i64 provider, slice<MixContents> entries)
    -> DbWriteResult<empty>;

[[pmacro::invoke("qcm::entity_input", DynamicInput, DynamicFields, Dynamic, {
    is_external: bool,
    remote_play_count: i64,
    remote_last_played_at: Option<Timestamp>,
    favorite_at: Option<Timestamp>,
},
                 {
                     favorite: Patch<bool>,
                 })]];

auto write_legacy_dynamics(sqlito::Transaction& tx, i64 provider, slice<Dynamic> entries,
                           slice<String> include, slice<String> exclude) -> DbWriteResult<empty>;
// The caller owns the transaction, the mask prevalidation and the favorite guard.
auto write_dynamics(sqlito::Transaction& tx, i64 provider, slice<DynamicInput> entries,
                    const Mask& mask, const SyncGuard& guard) -> DbWriteResult<Vec<Dynamic>>;
auto record_play(sqlito::Transaction& tx, i64 song_id, Option<i64> album_id, Timestamp timestamp)
    -> DbWriteResult<empty>;
// Ensures a dynamic row exists for every item allocated in this batch.
auto ensure_dynamic_rows(sqlito::Transaction& tx, slice<i64> item_ids) -> DbWriteResult<empty>;

[[pmacro::invoke("qcm::entity_input", RemoteMixInput, RemoteMixFields, RemoteMix, {
    name: String,
    description: Option<String>,
    track_count: i32,
    mix_type: String,
})]];
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", decode_only, ignore_unknown)]] RadioQueueInput {
    String native_id;
    String name;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> description;
};

// The caller owns the transaction and runs the write stages in batch order.
auto write_remote_mix(sqlito::Transaction& tx, i64 provider, const RemoteMixInput& input,
                      const Mask& mask) -> DbWriteResult<RemoteMix>;
auto sync_remote_mix_cache(sqlito::Transaction& tx, const RemoteMix& remote,
                           bool validate_track_count = true) -> DbWriteResult<empty>;
auto write_legacy_remote_models(sqlito::Transaction& tx, i64 provider, slice<RemoteMix> entries,
                                const Mask& mask) -> DbWriteResult<Vec<i64>>;
auto write_legacy_remote_mixes(sqlito::Transaction& tx, i64 provider, slice<RemoteMix> entries,
                               const Mask& mask) -> DbWriteResult<Vec<i64>>;
auto write_legacy_radio_queues(sqlito::Transaction& tx, i64 provider,
                               slice<RadioQueueInput> entries, Timestamp now)
    -> DbWriteResult<Vec<i64>>;
auto read_remote_mix(sqlito::Transaction& tx, i64 id) -> DbWriteResult<RemoteMix>;

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]]
BatchOptions {
    [[pmacro::helper("serde::derive", default)]]
    Mask albums;
    [[pmacro::helper("serde::derive", default)]]
    Mask artists;
    [[pmacro::helper("serde::derive", default)]]
    Mask songs;
    [[pmacro::helper("serde::derive", default)]]
    Mask remote_mixes;
    [[pmacro::helper("serde::derive", default)]]
    Mask dynamics;
};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]] SyncBatch {
    [[pmacro::helper("serde::derive", default)]]
    Vec<LibraryInput> libraries;
    [[pmacro::helper("serde::derive", default)]]
    Vec<AlbumInput> albums;
    [[pmacro::helper("serde::derive", default)]]
    Vec<ArtistInput> artists;
    [[pmacro::helper("serde::derive", default)]]
    Vec<SongInput> songs;
    [[pmacro::helper("serde::derive", default)]]
    Vec<RemoteMixInput> remote_mixes;
    [[pmacro::helper("serde::derive", default)]]
    Vec<DynamicInput> dynamics;
    [[pmacro::helper("serde::derive", default)]]
    Vec<ImageInput> images;
    [[pmacro::helper("serde::derive", default)]]
    Vec<SongAlbumInput> song_albums;
    [[pmacro::helper("serde::derive", default)]]
    Vec<RelationInput> album_artists;
    [[pmacro::helper("serde::derive", default)]]
    Vec<RelationInput> song_artists;
    [[pmacro::helper("serde::derive", default)]]
    Vec<MixContents> remote_mix_contents;
    [[pmacro::helper("serde::derive", default)]]
    BatchOptions options;
};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", encode_only)]]
SyncResult {
    Vec<Library>   libraries;
    Vec<Item>      items;
    Vec<Album>     albums;
    Vec<Artist>    artists;
    Vec<Song>      songs;
    Vec<RemoteMix> remote_mixes;
};

// The caller owns the transaction, coordinator guard and generation checks.
auto sync_batch(sqlito::Transaction& tx, i64 provider, const SyncBatch& batch,
                const SyncGuard& guard, usize encoded_bytes) -> DbWriteResult<SyncResult>;
} // namespace qcm::backend
