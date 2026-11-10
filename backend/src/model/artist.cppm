module;
#include <rstd/macro.hpp>
export module qcm.backend:model.artist;
export import rstd;
export import rstd.serde;
export import :db.timestamp;
import liorm;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("artist"), primary("artist_pk")),
         pmacro::helper("serde::derive", rename("Artist"), ignore_unknown)]] Artist {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> sort_name;
    [[pmacro::helper("liorm::table", storage(text)), pmacro::helper("serde::derive", default)]]
    String description;
    [[pmacro::helper("liorm::table", storage(integer)), pmacro::helper("serde::derive", default)]]
    i32 album_count;
    [[pmacro::helper("liorm::table", storage(integer)), pmacro::helper("serde::derive", default)]]
    i32 music_count;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> added_at;
};
} // namespace qcm::backend
