module;
#include "Qcm/qml/enum.hpp"
#include "core/log.h"

export module qcm:qml.enums;
export import qcm.core;
export import qcm.helper;

export namespace qcm::enums
{
using qcm::enums::ToastFlags;

using qcm::enums::qt_getEnumMetaObject;
using qcm::enums::qt_getEnumName;

using qcm::enums::ApiStatus;
using qcm::enums::AudioQuality;
using qcm::enums::CollectionType;
using qcm::enums::DisplayMode;
using qcm::enums::ImageQuality;
using qcm::enums::ImageType;
using qcm::enums::ItemType;
using qcm::enums::LoopMode;
using qcm::enums::ManipulateMixAction;
using qcm::enums::PlaybackState;
using qcm::enums::PluginBasicPage;
using qcm::enums::ProxyType;
using qcm::enums::RecordAction;
using qcm::enums::SearchLocation;
using qcm::enums::SearchType;
using qcm::enums::SpecialRoute;
using qcm::enums::SyncListType;
using qcm::enums::ToastFlag;

} // namespace qcm::enums

template<>
struct rstd::Impl<rstd::str_::FromStr, qcm::enums::ItemType> {
    using Err  = int;
    using Self = qcm::enums::ItemType;
    static auto from_str(ref_str str) -> rstd::Result<Self, Err> {
        return Ok(qcm::enums::item_type_from_str({ (char const*)str.data(), str.size() }));
    }
};

template<>
struct rstd::Impl<rstd::str_::FromStr, qcm::enums::ImageType> {
    using Err  = int;
    using Self = qcm::enums::ImageType;
    static auto from_str(ref_str str) -> rstd::Result<Self, Err> {
        if (str == "Primary") return Ok(Self::ImagePrimary);
        if (str == "Backdrop") return Ok(Self::ImageBackdrop);
        if (str == "Banner") return Ok(Self::ImageBanner);
        if (str == "Thumb") return Ok(Self::ImageThumb);
        if (str == "Logo") return Ok(Self::ImageLogo);
        return Ok(Self::ImagePrimary);
    }
};

template<>
struct rstd::Impl<rstd::fmt::Display, qcm::enums::ItemType> : rstd::ImplBase<qcm::enums::ItemType> {
    auto fmt(rstd::fmt::Formatter& f) const -> bool {
        std::string_view name = qcm::enums::item_type_to_str(this->self());
        return f.write_raw((const u8*)name.data(), name.size());
    }
};

template<>
struct rstd::Impl<rstd::fmt::Display, qcm::enums::ImageType>
    : rstd::ImplBase<qcm::enums::ImageType> {
    auto fmt(rstd::fmt::Formatter& f) const -> bool {
        std::string_view name;
        using ImageType = qcm::enums::ImageType;
        switch (this->self()) {
        case ImageType::ImagePrimary: name = "Primary"; break;
        case ImageType::ImageBackdrop: name = "Backdrop"; break;
        case ImageType::ImageBanner: name = "Banner"; break;
        case ImageType::ImageThumb: name = "Thumb"; break;
        case ImageType::ImageLogo: name = "Logo"; break;
        }
        return f.write_raw((const u8*)name.data(), name.size());
    }
};

// IMPL_CONVERT(std::string_view, qcm::enums::CollectionType) {
//     switch (in) {
//     case in_type::CTAlbum: {
//         out = "album"sv;
//         break;
//     }
//     case in_type::CTArtist: {
//         out = "artist"sv;
//         break;
//     }
//     case in_type::CTPlaylist: {
//         out = "playlist"sv;
//         break;
//     }
//     case in_type::CTRadio: {
//         out = "djradio"sv;
//         break;
//     }
//     default: {
//         _assert_rel_(false);
//     }
//     }
// }