#pragma once

#include <QtQml/qqmlregistration.h>
#include <QtQml/QQmlProperty>
#include <QtQml/QQmlPropertyMap>

#include "Qcm/message/item_id.hpp"

namespace qcm::model
{
extern auto common_extra(model::ItemId id) -> QQmlPropertyMap*;
}

// Per-type macros used by post-processed protobuf headers. The hook script
// rewrites `QML_VALUE_TYPE(<name>)` to `QCM_MSG_MODEL_<name>` for the types
// that need extra properties injected.

#define QCM_MSG_INJECT_ITEM_ID(ITEM_TYPE_ENUM)                                    \
public:                                                                           \
    Q_INVOKABLE qcm::model::ItemId itemId() const {                               \
        return qcm::model::ItemId { qcm::enums::ItemType::ITEM_TYPE_ENUM,         \
                                    static_cast<qint64>(this->id_proto()) };      \
    }                                                                             \
    Q_INVOKABLE QQmlPropertyMap* extra() const { return common_extra(itemId()); } \
    void                         setItemId(const qcm::model::ItemId& v) {         \
        this->setId_proto(static_cast<QtProtobuf::int64>(v.id()));                \
    }                                                                             \
                                                                                  \
private:

// Each entry: QML_VALUE_TYPE + extra injection. The hook rewrites the
// `QML_VALUE_TYPE(...)` found in generated headers to one of these.
#define QCM_MSG_MODEL_album      QML_VALUE_TYPE(album) QCM_MSG_INJECT_ITEM_ID(ItemAlbum)
#define QCM_MSG_MODEL_artist     QML_VALUE_TYPE(artist) QCM_MSG_INJECT_ITEM_ID(ItemAlbumArtist)
#define QCM_MSG_MODEL_mix        QML_VALUE_TYPE(mix) QCM_MSG_INJECT_ITEM_ID(ItemMix)
#define QCM_MSG_MODEL_radioQueue QML_VALUE_TYPE(radioQueue) QCM_MSG_INJECT_ITEM_ID(ItemRadioQueue)

#define QCM_MSG_MODEL_providerStatus \
    QML_VALUE_TYPE(providerStatus) QCM_MSG_INJECT_ITEM_ID(ItemProvider)

#define QCM_MSG_MODEL_song                                                   \
    QML_VALUE_TYPE(song)                                                     \
    QCM_MSG_INJECT_ITEM_ID(ItemSong)                                         \
public:                                                                      \
    Q_INVOKABLE auto albumItemId() const -> qcm::model::ItemId {             \
        return qcm::model::ItemId { enums::ItemType::ItemAlbum, albumId() }; \
    }                                                                        \
    Q_INVOKABLE auto albumName() const -> QString {                          \
        auto ex = common_extra(itemId());                                    \
        if (ex) {                                                            \
            auto al  = ex->value("album");                                   \
            auto map = al.toMap();                                           \
            return map.value("name", {}).toString();                         \
        }                                                                    \
        return {};                                                           \
    }                                                                        \
                                                                             \
private:
