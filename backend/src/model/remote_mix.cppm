export module qcm.backend:model.remote_mix;
export import rstd;
export import rstd.serde;
import liorm;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("remote_mix"), primary("remote_mix_pk")),
         pmacro::helper("serde::derive", rename("RemoteMix"), ignore_unknown)]] RemoteMix {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> description;
    [[pmacro::helper("liorm::table", storage(integer), nullable)]]
    i32 track_count;
    [[pmacro::helper("liorm::table", storage(text), nullable)]]
    String mix_type;
};
} // namespace qcm::backend
