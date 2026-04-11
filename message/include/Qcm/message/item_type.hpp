#pragma once
#include <string_view>

namespace qcm
{
namespace enums
{
enum class ItemType
{
    ItemInvalid = 0,

    ItemProvider = 1,
    ItemLibrary  = 2,

    ItemAlbum       = 51,
    ItemAlbumArtist = 52,
    ItemArtist      = 53,
    ItemMix         = 54,
    ItemRadio       = 55,
    ItemRadioQueue  = 56,

    ItemSong    = 101,
    ItemProgram = 102,
};

auto item_type_to_str(ItemType t) -> std::string_view;
auto item_type_from_str(std::string_view) -> ItemType;

} // namespace enums

} // namespace qcm