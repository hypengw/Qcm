#pragma once
#include <QQmlEngine>
#include "qcm_interface/export.h"

namespace qcm
{
namespace enums
{
QCM_INTERFACE_API Q_NAMESPACE QML_ELEMENT

    enum class QCM_INTERFACE_API ApiStatus {
        Uninitialized = 0,
        Querying,
        Finished,
        Error
    };
Q_ENUM_NS(ApiStatus);

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

enum class SpecialRoute
{
    SRQueue = 0,
    SRSetting,
    SRAbout,
    SRLogin,
};
Q_ENUM_NS(SpecialRoute)
} // namespace enums

} // namespace qcm