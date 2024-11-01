#pragma once

#include <QQmlEngine>
#include "qcm_interface/macro.h"
#include "qcm_interface/item_id.h"

namespace qcm::model
{

struct Playlist {
    Q_GADGET_EXPORT(QCM_INTERFACE_API)
    QML_VALUE_TYPE(t_playlist)
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(QString, description, description)
    GADGET_PROPERTY_DEF(QDateTime, createIime, createTime)
    GADGET_PROPERTY_DEF(QDateTime, updateTime, updateTime)
    GADGET_PROPERTY_DEF(qint32, playCount, playCount)
    GADGET_PROPERTY_DEF(qint32, trackCount, trackCount)
    GADGET_PROPERTY_DEF(ItemId, userId, userId)
    GADGET_PROPERTY_DEF(std::vector<QString>, tags, tags)

    static constexpr QStringView Select { uR"(
    playlist.itemId, 
    playlist.name, 
    playlist.picUrl, 
    playlist.description,
    playlist.trackCount,
    playlist.playCount,
    playlist.updateTime,
    playlist.userId
)" };

    std::strong_ordering operator<=>(const Playlist&) const = default;
};

} // namespace qcm::model