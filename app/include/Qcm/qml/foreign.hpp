#pragma once

#include "kstore/qt/meta_list_model.hpp"
#include "kstore/qt/gadget_model.hpp"
#include "mpris/mediaplayer2.h"

#include <QtQml/QQmlEngine>

namespace qcm
{
struct MetaListForeign {
    Q_GADGET
    QML_FOREIGN(kstore::QMetaListModel)
    QML_NAMED_ELEMENT(QMetaListModel)
    QML_UNCREATABLE("")
};

struct GadgetListForeign {
    Q_GADGET
    QML_FOREIGN(kstore::QGadgetListModel)
    QML_NAMED_ELEMENT(QGadgetListModel)
    QML_UNCREATABLE("")
};

struct MprisMediaPlayerForeign {
    Q_GADGET
    QML_FOREIGN(mpris::MediaPlayer2)
    QML_NAMED_ELEMENT(MprisMediaPlayer)
    QML_UNCREATABLE("uncreatable")
};

} // namespace qcm
