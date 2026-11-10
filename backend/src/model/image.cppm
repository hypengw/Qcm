export module qcm.backend:model.image;
export import rstd;
export import :datetime;
import liorm;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
enum class [[pmacro::derive(
    "liorm::sqlite_enum", "rstd::Debug", "rstd::Display", "rstd::EnumIter", "rstd::TryFrom",
    "rstd::FromStr", "serde::derive")]] [[pmacro::helper("rstd::FromStr", ascii_case_insensitive),
                                          pmacro::helper("serde::derive", rename("ImageType"),
                                                         string_enum, decode_only)]] ImageType : int
{
    Primary  = 0,
    Backdrop = 1,
    Banner   = 2,
    Thumb    = 3,
    Logo     = 4
};
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("image"), primary("image_pk")),
         pmacro::helper("serde::derive", rename("Image"), decode_only, ignore_unknown)]] Image {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 item_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    ImageType image_type;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> native_id;
    [[pmacro::helper("liorm::table", storage(text), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<String> db;
    [[pmacro::helper("liorm::table", storage(text)), pmacro::helper("serde::derive", default)]]
    String fresh;
    [[pmacro::helper("liorm::table", storage(text)),
      pmacro::helper("serde::derive", default(datetime_now))]]
    DatabaseDateTime timestamp;
};
} // namespace qcm::backend
