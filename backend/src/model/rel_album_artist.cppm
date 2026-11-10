export module qcm.backend:model.rel_album_artist;
export import rstd;
export import rstd.serde;
import liorm;
export import :db.timestamp;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("rel_album_artist"), primary("rel_album_artist_pk")),
         pmacro::helper("serde::derive", rename("RelAlbumArtist"), decode_only, ignore_unknown)]]
RelAlbumArtist {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 album_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 artist_id;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp update_at;
};
} // namespace qcm::backend
