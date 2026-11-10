export module qcm.backend:model.mix_model;
export import rstd;
export import rstd.serde;
import liorm;
export import :db.timestamp;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class [[pmacro::derive("liorm::sqlite_enum", "serde::derive"),
             pmacro::helper("serde::derive", rename("MixType"), string_enum,
                            decode_only)]] MixType : int
{
    Normal = 0,
    Link   = 1,
    Cache  = 2
};
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("mix"), primary("mix_pk")),
         pmacro::helper("serde::derive", rename("Mix"), decode_only, ignore_unknown)]] Mix {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i32 track_count;
    [[pmacro::helper("liorm::table", storage(integer))]]
    MixType mix_type;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> sort_name;
    [[pmacro::helper("liorm::table", storage(text)), pmacro::helper("serde::derive", default)]]
    String description;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<i64> remote_id;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp create_at;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp update_at;
    [[pmacro::helper("liorm::table", storage(integer)), pmacro::helper("serde::derive", default)]]
    Timestamp content_update_at;
};
} // namespace qcm::backend
