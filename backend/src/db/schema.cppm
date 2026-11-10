export module qcm.backend:db.schema;
export import liorm;
import :model.remote_mix;
import :model.mix_model;
import :model.rel_mix_song;
import :model.item;
import :model.library;
import :model.artist;
import :model.album;
import :model.song;
import :model.rel_album_artist;
import :model.rel_song_artist;
import :model.dynamic;
import :model.image;
using namespace rstd::prelude;

export namespace qcm::backend
{
namespace db_relations
{
inline constexpr auto item_library =
    liorm::schema::Relation<"item_library", Item, Library, &Item::library_id> {};
inline constexpr auto artist_item =
    liorm::schema::Relation<"artist_item", Artist, Item, &Artist::id> {};
inline constexpr auto album_item =
    liorm::schema::Relation<"album_item", Album, Item, &Album::id> {};
inline constexpr auto song_item = liorm::schema::Relation<"song_item", Song, Item, &Song::id> {};
inline constexpr auto song_album =
    liorm::schema::Relation<"song_album", Song, Album, &Song::album_id> {};
inline constexpr auto remote_mix_item =
    liorm::schema::Relation<"remote_mix_item", RemoteMix, Item, &RemoteMix::id> {};
inline constexpr auto mix_remote =
    liorm::schema::Relation<"mix_remote", Mix, RemoteMix, &Mix::remote_id> {};
inline constexpr auto rel_mix_song_mix =
    liorm::schema::Relation<"rel_mix_song_mix", RelMixSong, Mix, &RelMixSong::mix_id> {};
inline constexpr auto rel_mix_song_song =
    liorm::schema::Relation<"rel_mix_song_song", RelMixSong, Song, &RelMixSong::song_id> {};
inline constexpr auto rel_album_artist_album =
    liorm::schema::Relation<"rel_album_artist_album", RelAlbumArtist, Album,
                            &RelAlbumArtist::album_id> {};
inline constexpr auto rel_album_artist_artist =
    liorm::schema::Relation<"rel_album_artist_artist", RelAlbumArtist, Artist,
                            &RelAlbumArtist::artist_id> {};
inline constexpr auto rel_song_artist_song =
    liorm::schema::Relation<"rel_song_artist_song", RelSongArtist, Song,
                            &RelSongArtist::song_id> {};
inline constexpr auto rel_song_artist_artist =
    liorm::schema::Relation<"rel_song_artist_artist", RelSongArtist, Artist,
                            &RelSongArtist::artist_id> {};
inline constexpr auto dynamic_item =
    liorm::schema::Relation<"dynamic_item", Dynamic, Item, &Dynamic::id> {};
inline constexpr auto dynamic_album =
    liorm::schema::Relation<"dynamic_album", Dynamic, Album, &Dynamic::id> {};
inline constexpr auto dynamic_song =
    liorm::schema::Relation<"dynamic_song", Dynamic, Song, &Dynamic::id> {};
inline constexpr auto image_item =
    liorm::schema::Relation<"image_item", Image, Item, &Image::item_id> {};
inline constexpr auto image_album =
    liorm::schema::Relation<"image_album", Image, Album, &Image::item_id> {};
inline constexpr auto image_song =
    liorm::schema::Relation<"image_song", Image, Song, &Image::item_id> {};
inline constexpr auto image_artist =
    liorm::schema::Relation<"image_artist", Image, Artist, &Image::item_id> {};
inline constexpr auto image_remote_mix =
    liorm::schema::Relation<"image_remote_mix", Image, RemoteMix, &Image::item_id> {};
} // namespace db_relations

inline constexpr auto database_tables = liorm::schema::define([](liorm::schema::SchemaBuilder& b) {
    auto library = liorm::schema::register_table<Library>(b);
    auto item    = liorm::schema::register_table<Item>(b);
    db_relations::item_library.register_in(b, item, library);
    auto artist = liorm::schema::register_table<Artist>(b);
    db_relations::artist_item.register_in(b, artist, item);
    auto album = liorm::schema::register_table<Album>(b);
    db_relations::album_item.register_in(b, album, item);
    auto song = liorm::schema::register_table<Song>(b);
    db_relations::song_item.register_in(b, song, item);
    db_relations::song_album.register_in(b, song, album);
    auto remote = liorm::schema::register_table<RemoteMix>(b);
    db_relations::remote_mix_item.register_in(b, remote, item);

    auto cache = liorm::schema::register_table<Mix>(b);
    db_relations::mix_remote.register_in(b, cache, remote);

    auto relation = liorm::schema::register_table<RelMixSong>(b);
    db_relations::rel_mix_song_mix.register_in(b, relation, cache);
    db_relations::rel_mix_song_song.register_in(b, relation, song);
    auto album_artists = liorm::schema::register_table<RelAlbumArtist>(b);
    db_relations::rel_album_artist_album.register_in(b, album_artists, album);
    db_relations::rel_album_artist_artist.register_in(b, album_artists, artist);
    auto song_artists = liorm::schema::register_table<RelSongArtist>(b);
    db_relations::rel_song_artist_song.register_in(b, song_artists, song);
    db_relations::rel_song_artist_artist.register_in(b, song_artists, artist);
    auto dynamic = liorm::schema::register_table<Dynamic>(b);
    db_relations::dynamic_item.register_in(b, dynamic, item);
    db_relations::dynamic_album.register_in(b, dynamic, album);
    db_relations::dynamic_song.register_in(b, dynamic, song);
    auto image = liorm::schema::register_table<Image>(b);
    db_relations::image_item.register_in(b, image, item);
    db_relations::image_album.register_in(b, image, album);
    db_relations::image_song.register_in(b, image, song);
    db_relations::image_artist.register_in(b, image, artist);
    db_relations::image_remote_mix.register_in(b, image, remote);
});

} // namespace qcm::backend
