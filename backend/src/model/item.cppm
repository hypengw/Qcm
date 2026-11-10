export module qcm.backend:model.item;
export import liorm;
export import sqlito;
export import rstd.serde;
export import :db.timestamp;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
enum class [[pmacro::derive(
    "liorm::sqlite_enum", "rstd::Debug", "rstd::Display", "rstd::EnumIter", "rstd::TryFrom",
    "rstd::FromStr", "serde::derive")]] [[pmacro::helper("rstd::FromStr", ascii_case_insensitive),
                                          pmacro::helper("serde::derive", rename("ItemType"),
                                                         string_enum)]] ItemType : int
{
    Unspecified [[pmacro::helper("rstd::Debug", rename("UnSpecified"))]]
    [[pmacro::helper("rstd::Display", rename("UnSpecified"))]] [[pmacro::helper(
        "rstd::FromStr", rename("UnSpecified"))]] [[pmacro::helper("serde::derive",
                                                                   rename("UnSpecified"))]] = 0,

    Provider    = 1,
    Library     = 2,
    Album       = 51,
    AlbumArtist = 52,
    Artist      = 53,
    Mix         = 54,
    Radio       = 55,
    Song        = 101,
    Program     = 102
};
template<class T>
using ItemTypeConversionError = rstd::derive::TryFromEnumError<ItemType, T>;
struct [[pmacro::derive("liorm::table", "serde::derive"),
         pmacro::helper("liorm::table", name("item"), primary("item_pk")),
         pmacro::helper("serde::derive", rename("Item"), ignore_unknown)]] Item {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String native_id;
    [[pmacro::helper("liorm::table", storage(integer), nullable),
      pmacro::helper("serde::derive", default)]]
    Option<i64> library_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 provider_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    ItemType type;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp create_at;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp update_at;
    [[pmacro::helper("liorm::table", storage(integer)),
      pmacro::helper("serde::derive", default(Timestamp::now))]]
    Timestamp last_sync_at;
};

} // namespace qcm::backend
