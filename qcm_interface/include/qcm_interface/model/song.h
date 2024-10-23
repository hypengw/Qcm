#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{

struct Song {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_VALUE_TYPE(t_song)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(ItemId, albumId, albumId)
    GADGET_PROPERTY_DEF(qint32, trackNumber, trackNumber)
    GADGET_PROPERTY_DEF(QDateTime, duration, duration)
    GADGET_PROPERTY_DEF(bool, canPlay, canPlay)
    GADGET_PROPERTY_DEF(QString, coverUrl, coverUrl)
    GADGET_PROPERTY_DEF(QStringList, tags, tags)
    GADGET_PROPERTY_DEF(qreal, popularity, popularity)

    GADGET_PROPERTY_DEF(QVariant, source, source)
    GADGET_PROPERTY_DEF(ItemId, sourceId, sourceId)

    static constexpr QStringView Select { uR"(
    song.itemId, 
    song.name, 
    COALESCE(song.coverUrl, album.picUrl) AS picUrl,
    song.canPlay,
)" };

    // GATGET_LIST_PROPERTY(Artist, artists, artists)

    bool operator==(const Song&) const = default;
};

} // namespace qcm::model