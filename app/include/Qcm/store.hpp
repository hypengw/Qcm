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
    using mix_store    = meta_model::ItemTrait<qcm::model::Mix>::store_type;

    using album_item = album_store::store_item_type;
    using song_item  = song_store::store_item_type;

    album_store  albums;
    song_store   songs;
    artist_store artists;
    mix_store    mixes;
};

namespace model
{
extern const std::set<QStringView> AlbumJsonFields;
extern const std::set<QStringView> ArtistJsonFields;
extern const std::set<QStringView> MixJsonFields;
extern const std::set<QStringView> SongJsonFields;
} // namespace model

template<typename T>
auto merge_store_extra(T& store, i64 key, const google::protobuf::Struct& in) {
    if (auto extend = store.query_extend(key); extend) {
        std::set<QStringView> const* json_fields { nullptr };
        if constexpr (std::same_as<T, AppStore::album_store>) {
            json_fields = &model::AlbumJsonFields;
        } else if constexpr (std::same_as<T, AppStore::song_store>) {
            json_fields = &model::SongJsonFields;
        } else if constexpr (std::same_as<T, AppStore::artist_store>) {
            json_fields = &model::ArtistJsonFields;
        } else if constexpr (std::same_as<T, AppStore::mix_store>) {
            json_fields = &model::MixJsonFields;
        } else {
            static_assert(false);
        }
        msg::merge_extra(*(extend->extra), in, *json_fields);
    }
}

} // namespace qcm
