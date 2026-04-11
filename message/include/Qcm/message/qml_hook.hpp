#pragma once

#include <QtQml/qqmlregistration.h>

#include "Qcm/model/item_id.hpp"

// Per-type macros used by post-processed protobuf headers. The hook script
// rewrites `QML_VALUE_TYPE(<name>)` to `QCM_MSG_MODEL_<name>` for the types
// that need extra properties injected.

#define QCM_MSG_INJECT_ITEM_ID(ITEM_TYPE_ENUM)                                           \
public:                                                                                  \
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId FINAL)              \
    qcm::model::ItemId itemId() const {                                                  \
        return qcm::model::ItemId { qcm::enums::ItemType::ITEM_TYPE_ENUM,                \
                                    static_cast<qint64>(this->id_proto()) };             \
    }                                                                                    \
    void setItemId(const qcm::model::ItemId& v) {                                        \
        this->setId_proto(static_cast<QtProtobuf::int64>(v.id()));                       \
    }                                                                                    \
                                                                                         \
private:

// Each entry: QML_VALUE_TYPE + extra injection. The hook rewrites the
// `QML_VALUE_TYPE(...)` found in generated headers to one of these.
#define QCM_MSG_MODEL_album          QML_VALUE_TYPE(album)          QCM_MSG_INJECT_ITEM_ID(ItemAlbum)
#define QCM_MSG_MODEL_song           QML_VALUE_TYPE(song)           QCM_MSG_INJECT_ITEM_ID(ItemSong)
#define QCM_MSG_MODEL_artist         QML_VALUE_TYPE(artist)         QCM_MSG_INJECT_ITEM_ID(ItemAlbumArtist)
#define QCM_MSG_MODEL_mix            QML_VALUE_TYPE(mix)            QCM_MSG_INJECT_ITEM_ID(ItemMix)
#define QCM_MSG_MODEL_providerStatus QML_VALUE_TYPE(providerStatus) QCM_MSG_INJECT_ITEM_ID(ItemProvider)
