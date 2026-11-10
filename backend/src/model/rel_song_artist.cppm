export module qcm.backend:model.rel_song_artist;
export import rstd;
export import rstd.serde;
import liorm;
export import :db.timestamp;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("rel_song_artist"), primary("rel_song_artist_pk")),
         pmacro::helper("serde::derive", rename("RelSongArtist"), decode_only, ignore_unknown)]]
RelSongArtist {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 song_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 artist_id;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp update_at;
};
} // namespace qcm::backend
