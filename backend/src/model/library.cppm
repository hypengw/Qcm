module;
export module qcm.backend:model.library;
export import :datetime;
export import rstd.serde;
import liorm;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("library"), primary("library_pk")),
         pmacro::helper("serde::derive", rename("Library"), ignore_unknown)]] Library {
    [[pmacro::helper("liorm::table", name("library_id"), storage(integer), primary),
      pmacro::helper("serde::derive", rename("library_id"))]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 provider_id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String native_id;
    [[pmacro::helper("liorm::table", storage(text)),
      pmacro::helper("serde::derive", default(datetime_now))]]
    DatabaseDateTime edit_time;
};
} // namespace qcm::backend
