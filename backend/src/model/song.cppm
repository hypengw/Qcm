export module qcm.backend:model.song;
export import rstd;
export import rstd.json;
export import rstd.serde;
export import :db.timestamp;
import liorm;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("song"), primary("song_pk")),
         pmacro::helper("serde::derive", rename("Song"), ignore_unknown, validate(Song::validate))]]
Song {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> sort_name;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<i64> album_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i32 track_number;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i32 disc_number;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 duration;
    [[pmacro::helper("liorm::table", storage(boolean)), pmacro::helper("serde::derive", default)]]
    bool can_play { true };
    [[pmacro::helper("liorm::table", storage(text)), pmacro::helper("serde::derive", default)]]
    rstd::json::Value tags { rstd::json::Value::Array(rstd::json::Array {}) };
    [[pmacro::helper("liorm::table", storage(real)), pmacro::helper("serde::derive", default)]]
    f64 popularity;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> publish_time;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> added_at;

    static auto validate(const Song&) -> Result<empty, ref<str>>;
};
auto Song::validate(const Song& value) -> Result<empty, ref<str>> {
    if (! value.popularity.is_finite()) return Err("popularity must be finite"_str);
    return Ok(empty {});
}
} // namespace qcm::backend
