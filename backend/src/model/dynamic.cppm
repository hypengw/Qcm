export module qcm.backend:model.dynamic;
export import rstd;
export import rstd.serde;
import liorm;
export import :db.timestamp;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("dynamic"), primary("dynamic_pk")),
         pmacro::helper("serde::derive", rename("Dynamic"), decode_only, ignore_unknown)]] Dynamic {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(boolean)), pmacro::helper("serde::derive", default)]]
    bool is_external;
    [[pmacro::helper("liorm::table", storage(integer)), pmacro::helper("serde::derive", default)]]
    i64 play_count;
    [[pmacro::helper("liorm::table", storage(integer)), pmacro::helper("serde::derive", default)]]
    i64 remote_play_count;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<i64> last_position;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> last_played_at;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> remote_last_played_at;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<Timestamp> favorite_at;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp update_at;
};
} // namespace qcm::backend
