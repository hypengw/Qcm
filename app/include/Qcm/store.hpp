#pragma once

#include "Qcm/model/share_store.hpp"

#include "Qcm/backend_msg.hpp"

namespace qcm
{

class AppStore : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Store)
    QML_SINGLETON
public:
    AppStore(QObject* parent);
    ~AppStore();
    static auto      instance() -> AppStore*;
    static AppStore* create(QQmlEngine*, QJSEngine*);
    // make qml prefer create
    AppStore() = delete;

    Q_INVOKABLE QQmlPropertyMap* extra(model::ItemId id) const;

    using album_store  = meta_model::ItemTrait<qcm::model::Album>::store_type;
    using song_store   = meta_model::ItemTrait<qcm::model::Song>::store_type;
    using artist_store = meta_model::ItemTrait<qcm::model::Artist>::store_type;

    using album_item = album_store::store_item_type;
    using song_item  = song_store::store_item_type;

    album_store  albums;
    song_store   songs;
    artist_store artists;
};

} // namespace qcm
