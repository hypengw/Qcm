export module qcm.backend:model.home;
export import :db.sync_batch.item_reference;
export import :model.album;
export import :model.artist;
export import :model.mix_model;
export import :model.song;
export import rstd;
export import rstd.serde;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", rename("HomeBlockStyle"), string_enum,
                            decode_only)]] HomeBlockStyle
{
    Unspecified,
    Grid,
    Carousel,
    List
};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", rename("HomeBlock"),
                                                         decode_only, ignore_unknown)]] HomeBlock {
    String native_id;
    String title;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> subtitle;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> description;
    HomeBlockStyle style { HomeBlockStyle::Unspecified };
    ItemType       item_type { ItemType::Unspecified };
};
[[pmacro::invoke("rstd::Enum",
                 [[derive("serde::derive")]] [[helper("serde::derive", rename("HomeBlockContent"),
                                                      decode_only)]] enum HomeBlockContent {
                     Albums(items : Vec<Album>),
                     Mixes(items : Vec<Mix>),
                     Artists(items : Vec<Artist>),
                     Songs(items : Vec<Song>),
                 })]];
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("HomeBlockItems"), decode_only, ignore_unknown)]]
HomeBlockItems {
    HomeBlockContent content { HomeBlockContent::Albums(Vec<Album> {}) };
    [[pmacro::helper("serde::derive", default)]]
    i32 total {};
};
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", rename("HomeCardAction"), string_enum,
                            decode_only)]] HomeCardAction
{
    OpenItem,
    PlayItem,
    ActivateRadio
};
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", rename("HomeCardStyle"), string_enum,
                            decode_only)]] HomeCardStyle
{
    Media,
    Radio
};
[[pmacro::invoke("rstd::Enum", [[derive("serde::derive")]] [[helper(
                                   "serde::derive", rename("HomeCover"), adjacent, tag("type"),
                                   content("item"), decode_only)]] enum HomeCover {
                                   Target,
                                   Item(item : ItemRef),
                                   RadioCurrent,
                               })]];
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("HomePresentation"), decode_only)]]
HomePresentation {
    [[pmacro::helper("serde::derive", default)]]
    HomeCardStyle style { HomeCardStyle::Media };
    [[pmacro::helper("serde::derive", default)]]
    Option<String> title;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> subtitle;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> badge;
    [[pmacro::helper("serde::derive", default)]]
    HomeCover cover { HomeCover::Target() };
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("HomeEntry"), decode_only)]] HomeEntry {
    [[pmacro::helper("serde::derive", rename("entry_key"))]]
    String  key;
    ItemRef target { ItemRef::Id(i64()) };
    [[pmacro::helper("serde::derive", default)]]
    HomePresentation presentation;
    [[pmacro::helper("serde::derive", rename("primary_action"))]]
    HomeCardAction primary { HomeCardAction::OpenItem };
    [[pmacro::helper("serde::derive", rename("play_action"), default)]]
    Option<HomeCardAction> play;
};
} // namespace qcm::backend
