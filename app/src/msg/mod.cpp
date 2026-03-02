module;
#include <QString>
#include <QQmlPropertyMap>
#include <QQmlEngine>
#include "Qcm/msg/backend_msg.moc.h"
#include "Qcm/msg/store.moc.h"

#include "core/log.h"

module qcm;
import :msg;
import :global;
import qcm.log;

namespace qcm
{
void msg::merge_extra(QQmlPropertyMap& extra, const google::protobuf::Struct& in,
                      const std::set<QStringView>& is_json_field) {
    auto it  = in.fields().cbegin();
    auto end = in.fields().cend();
    for (; it != end; it++) {
        auto     key = it.key();
        QVariant val;
        if (is_json_field.contains(key)) {
            if (it.value().hasStringValue()) {
                auto json = QJsonDocument::fromJson(it.value().stringValue().toUtf8());
                val       = json.toVariant();
            } else {
                LOG_WARN("wrong field");
            }
        } else {
            val = rstd::into(it.value());
        }
        extra.insert(key, std::move(val));
    }
}

} // namespace qcm

auto rstd::Impl<rstd::convert::From<google::protobuf::Value>, QVariant>::from(
    google::protobuf::Value val) -> QVariant {
    using KindFields = google::protobuf::Value::KindFields;
    switch (val.kindField()) {
    case KindFields::ListValue: {
        QVariantList list;
        for (auto& el : val.listValue().values()) {
            list.push_back(rstd::into(el));
        }
        return list;
    }
    case KindFields::StructValue: {
        QVariantMap map;
        auto        it  = val.structValue().fields().cbegin();
        auto        end = val.structValue().fields().cend();
        for (; it != end; it++) {
            map.insert(it.key(), rstd::into(it.value()));
        }
        return map;
    }
    case KindFields::NumberValue: {
        return val.numberValue();
    }
    case KindFields::StringValue: {
        return val.stringValue();
    }
    case KindFields::BoolValue: {
        return val.boolValue();
    }
    case KindFields::NullValue:
    default: {
        return {};
    }
    }
}

auto rstd::Impl<rstd::convert::From<qcm::enums::ItemType>,
                qcm::msg::model::ItemTypeGadget::ItemType>::from(in_t t) -> out_t {
    return (out_t)(i32)(t);
}

namespace qcm::model
{

auto Song::albumName() const -> QString {
    auto ex = AppStore::instance()->extra(itemId());
    if (ex) {
        auto al  = ex->value("album");
        auto map = al.toMap();
        return map.value("name", {}).toString();
    }
    return {};
}
} // namespace qcm::model

auto qcm::model::common_extra(model::ItemId id) -> QQmlPropertyMap* {
    return AppStore::instance()->extra(id);
}

namespace qcm
{
AppStore::AppStore(QObject* parent)
    : QObject(parent),
      albums(mem_mgr().store_mem),
      songs(mem_mgr().store_mem),
      artists(mem_mgr().store_mem) {}
AppStore::~AppStore() {}
auto AppStore::instance() -> AppStore* {
    static auto the =
        GlobalStatic::instance()->add<AppStore>("store", new AppStore(nullptr), [](AppStore* p) {
            delete p;
        });
    return the;
}
AppStore* AppStore::create(QQmlEngine*, QJSEngine*) {
    auto self = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(self, QJSEngine::CppOwnership);
    return self;
}

auto AppStore::extra(model::ItemId item_id) const -> QQmlPropertyMap* {
    using ItemType = enums::ItemType;
    auto id        = item_id.id();
    switch (item_id.type()) {
    case ItemType::ItemAlbum: {
        if (auto extend = albums.query_extend(id)) {
            return extend->extra.get();
        }
        break;
    }
    case ItemType::ItemSong: {
        if (auto extend = songs.query_extend(id)) {
            return extend->extra.get();
        }
        break;
    }
    case ItemType::ItemAlbumArtist:
    case ItemType::ItemArtist: {
        if (auto extend = artists.query_extend(id)) {
            return extend->extra.get();
        }
        break;
    }
    default: {
        break;
    }
    }
    return nullptr;
}

const std::set<QStringView> model::AlbumJsonFields { u"artists", u"dynamic" };
const std::set<QStringView> model::ArtistJsonFields {};
const std::set<QStringView> model::MixJsonFields {};
const std::set<QStringView> model::SongJsonFields { u"artists", u"album", u"dynamic" };

} // namespace qcm

#include "Qcm/msg/backend_msg.moc.cpp"
#include "Qcm/msg/store.moc.cpp"