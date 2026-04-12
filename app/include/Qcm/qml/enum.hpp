#pragma once

#include <QQmlEngine>

#include "Qcm/message/item_type.hpp"
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

enum class DisplayMode
{
    DList = 0,
    DGrid,
    DCardGrid,
};
Q_ENUM_NS(DisplayMode)

enum class PluginBasicPage
{
    BPageLogin
};
Q_ENUM_NS(PluginBasicPage);

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

enum SpecialRoute
{
    SRQueue = 0,
    SRSetting,
    SRAbout,
    SRLogin,
    SRLoading,
    SRStatus,
    SRSearch,
    SRSync,
    SRMain,
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
    SearchAlbum = 0,
    SearchArtist,
    SearchSong,
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



enum class ItemTypeQml
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
Q_ENUM_NS(ItemTypeQml)


} // namespace enums

} // namespace qcm

// DECLARE_CONVERT(std::string_view, qcm::enums::CollectionType, QCM_INTERFACE_API);