export module qcm.backend:model.album;
export import rstd;
export import rstd.serde;
export import :db.timestamp;
import liorm;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
enum class [[pmacro::derive("liorm::sqlite_enum", "rstd::Debug", "rstd::Display", "rstd::EnumIter",
                            "rstd::TryFrom", "rstd::FromStr", "serde::derive"),
             pmacro::helper("serde::derive", rename("AlbumType"), string_enum)]] AlbumType : int
{
    Unknown     = 0,
    Album       = 1,
    EP          = 2,
    Single      = 3,
    Soundtrack  = 4,
    Compilation = 5,
    Live        = 6
};
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("album"), primary("album_pk")),
         pmacro::helper("serde::derive", rename("Album"), ignore_unknown)]] Album {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> sort_name;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i32 track_count;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i32 disc_count;
    [[pmacro::helper("liorm::table", storage(integer)), pmacro::helper("serde::derive", default)]]
    AlbumType type { AlbumType::Album };
    [[pmacro::helper("liorm::table", storage(integer)), pmacro::helper("serde::derive", default)]]
    i64 duration;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> language;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> description;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> company;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> publish_time;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> added_at;
};
} // namespace qcm::backend
