#pragma once
#include <QQmlEngine>
#include "qcm_interface/export.h"

namespace qcm
{
namespace enums
{
Q_NAMESPACE_EXPORT(QCM_INTERFACE_API)
QML_ELEMENT

enum class QCM_INTERFACE_API ApiStatus
{
    Uninitialized = 0,
    Querying,
    Finished,
    Error
};
Q_ENUM_NS(ApiStatus);

enum class QCM_INTERFACE_API LoopMode
{
    NoneLoop,
    SingleLoop,
    ListLoop,
    ShuffleLoop
};
Q_ENUM_NS(LoopMode)

enum class QCM_INTERFACE_API PluginBasicPage
{
    BPageLogin
};
Q_ENUM_NS(PluginBasicPage);

// enum class QCM_INTERFACE_API IdType
//{
//     IdTypeUnknown = 0,
//     IdTypeSong,
//     IdTypeAlbum,
//     IdTypeArtist,
//     IdTypePlaylist,
//     IdTypeUser,
//     IdTypeComment,
//     IdTypeDjradio,
//     IdTypeProgram
// };
// Q_ENUM_NS(IdType)

enum class QCM_INTERFACE_API ToastFlag
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
};
Q_ENUM_NS(SpecialRoute)

enum class CollectionType
{
    CTAlbum = 0,
    CTArtist,
    CTPlaylist,
    CTDjradio,
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
};
Q_ENUM_NS(RecordAction)
} // namespace enums

} // namespace qcm

DECLARE_CONVERT(std::string_view, qcm::enums::CollectionType, QCM_INTERFACE_API);