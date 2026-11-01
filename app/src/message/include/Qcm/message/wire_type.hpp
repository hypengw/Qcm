#pragma once
#include "Qcm/message/item_type.hpp"
#include "Qcm/message/model.qpb.h"

namespace qcm::identity
{
inline auto wireItemType(enums::ItemType type) -> msg::model::ItemTypeGadget::ItemType {
    using I = enums::ItemType;
    using W = msg::model::ItemTypeGadget::ItemType;
    switch (type) {
    case I::ItemProvider: return W::ITEM_TYPE_PROVIDER;
    case I::ItemLibrary: return W::ITEM_TYPE_LIBRARY;
    case I::ItemSong: return W::ITEM_TYPE_SONG;
    case I::ItemAlbum: return W::ITEM_TYPE_ALBUM;
    case I::ItemArtist: return W::ITEM_TYPE_ARTIST;
    case I::ItemAlbumArtist: return W::ITEM_TYPE_ALBUMARTIST;
    case I::ItemMix: return W::ITEM_TYPE_MIX;
    case I::ItemRadio:
    case I::ItemRadioQueue: return W::ITEM_TYPE_RADIO;
    case I::ItemProgram: return W::ITEM_TYPE_PROGRAM;
    default: return W::ITEM_TYPE_UNSPECIFIED;
    }
}
} // namespace qcm::identity
