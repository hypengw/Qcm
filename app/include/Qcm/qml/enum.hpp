#pragma once
#include <QQmlEngine>
#include "core/core.h"

namespace qcm
{
namespace enums
{
Q_NAMESPACE
QML_NAMED_ELEMENT(Enum)

enum class ApiStatus
{
    Uninitialized = 0,
    Querying,
    Finished,
    Error
};
Q_ENUM_NS(ApiStatus);

enum class LoopMode
{
    NoneLoop,
    SingleLoop,
    ListLoop,
    ShuffleLoop
};
Q_ENUM_NS(LoopMode)

enum class PluginBasicPage
{
    BPageLogin
};
Q_ENUM_NS(PluginBasicPage);

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

    ItemSong    = 101,
    ItemProgram = 102,
};
Q_ENUM_NS(ItemType)

enum class ImageType
{
    ImagePrimary  = 0,
    ImageBackdrop = 1,
    ImageBanner   = 2,
    ImageThumb    = 3,
    ImageLogo     = 4,
};
Q_ENUM_NS(ImageType)

enum class ToastFlag
{
    TFCloseable = 1,
    TFSave      = 1 << 1,
};
Q_ENUM_NS(ToastFlag);
Q_DECLARE_FLAGS(ToastFlags, ToastFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(ToastFlags);

enum class PlaybackState
{
    PlayingState = 0,
    PausedState,
    StoppedState,
};
Q_ENUM_NS(PlaybackState)

enum class AudioQuality
{
    AQStandard = 0,
    AQHigher,
    AQExhigh,
    AQLossless,
    AQHires
};
Q_ENUM_NS(AudioQuality)

enum class SpecialRoute
{
    SRQueue = 0,
    SRSetting,
    SRAbout,
    SRLogin,
    SRLoading,
    SRStatus,
    SRSearch,
    SRSync,
};
Q_ENUM_NS(SpecialRoute)

enum class CollectionType
{
    CTAlbum = 0,
    CTArtist,
    CTPlaylist,
    CTRadio,
};
Q_ENUM_NS(CollectionType)

enum class SyncListType
{
    CTArtistSong = 0,
    CTArtistAlbum,
};
Q_ENUM_NS(SyncListType)

enum class RecordAction
{
    RecordSwitch = 0,
    RecordNext,
    RecordPrev,
    RecordSwitchQueue
};
Q_ENUM_NS(RecordAction)

enum class ManipulateMixAction
{
    ManipulateMixAdd = 0,
    ManipulateMixDel,
};
Q_ENUM_NS(ManipulateMixAction)

enum class SearchLocation
{
    SearchLocal = 0,
    SearchOnline,
};
Q_ENUM_NS(SearchLocation)

enum class SearchType
{
    SearchSong = 0,
    SearchAlbum,
};
Q_ENUM_NS(SearchType)

enum class ImageQuality
{
    Img400px  = 400,
    Img800px  = 800,
    Img1200px = 1200,
    ImgAuto   = -1,
    ImgOrigin = -2,
};
Q_ENUM_NS(ImageQuality)

enum class ProxyType
{
    ProxyHttp    = 0,
    ProxyHttps2  = 3,
    ProxySocks4  = 4,
    ProxySocks5  = 5,
    ProxySocks4a = 6,
    ProxySocks5h = 7
};
Q_ENUM_NS(ProxyType)
} // namespace enums

} // namespace qcm

// DECLARE_CONVERT(std::string_view, qcm::enums::CollectionType, QCM_INTERFACE_API);

template<>
struct rstd::Impl<rstd::str::FromStr, qcm::enums::ItemType> {
    using Err  = int;
    using Self = qcm::enums::ItemType;
    static auto from_str(ref_str str) -> rstd::Result<Self, Err> {
        if (str == "Provider") return Ok(Self::ItemProvider);
        if (str == "Library") return Ok(Self::ItemLibrary);
        if (str == "Album") return Ok(Self::ItemAlbum);
        if (str == "AlbumArtist") return Ok(Self::ItemAlbumArtist);
        if (str == "Artist") return Ok(Self::ItemArtist);
        if (str == "Mix") return Ok(Self::ItemMix);
        if (str == "Radio") return Ok(Self::ItemRadio);
        if (str == "Song") return Ok(Self::ItemSong);
        if (str == "Program") return Ok(Self::ItemProgram);
        return Ok(Self::ItemInvalid);
    }
};

template<>
struct rstd::Impl<rstd::str::FromStr, qcm::enums::ImageType> {
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
struct std::formatter<qcm::enums::ItemType> : std::formatter<std::string_view> {
    template<class FmtContext>
    FmtContext::iterator format(qcm::enums::ItemType e, FmtContext& ctx) const {
        std::string_view name;
        using ItemType = qcm::enums::ItemType;
        switch (e) {
        case ItemType::ItemInvalid: name = "Invalid"; break;
        case ItemType::ItemProvider: name = "Provider"; break;
        case ItemType::ItemLibrary: name = "Library"; break;
        case ItemType::ItemAlbum: name = "Album"; break;
        case ItemType::ItemAlbumArtist: name = "AlbumArtist"; break;
        case ItemType::ItemArtist: name = "Artist"; break;
        case ItemType::ItemMix: name = "Mix"; break;
        case ItemType::ItemRadio: name = "Radio"; break;
        case ItemType::ItemSong: name = "Song"; break;
        case ItemType::ItemProgram: name = "Program"; break;
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};

template<>
struct std::formatter<qcm::enums::ImageType> : std::formatter<std::string_view> {
    template<class FmtContext>
    FmtContext::iterator format(qcm::enums::ImageType e, FmtContext& ctx) const {
        std::string_view name;
        using ImageType = qcm::enums::ImageType;
        switch (e) {
        case ImageType::ImagePrimary: name = "Primary"; break;
        case ImageType::ImageBackdrop: name = "Backdrop"; break;
        case ImageType::ImageBanner: name = "Banner"; break;
        case ImageType::ImageThumb: name = "Thumb"; break;
        case ImageType::ImageLogo: name = "Logo"; break;
        }
        return std::formatter<std::string_view>::format(name, ctx);
    }
};