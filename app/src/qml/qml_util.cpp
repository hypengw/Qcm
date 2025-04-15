#include "Qcm/qml/qml_util.hpp"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtQml/QJSValueIterator>

#include "core/core.h"
#include "core/qstr_helper.h"
#include "crypto/crypto.h"
#include "core/asio/basic.h"

#include "meta_model/qgadget_helper.hpp"
#include "Qcm/global.hpp"
#include "Qcm/app.hpp"
#include "Qcm/util/path.hpp"
#include "Qcm/backend.hpp"

namespace qcm::qml
{

Util::Util(std::monostate): QObject(nullptr) {}
Util::~Util() {}

Util* Util::create(QQmlEngine*, QJSEngine*) {
    Util* util = App::instance()->util();
    // not delete on qml
    QJSEngine::setObjectOwnership(util, QJSEngine::CppOwnership);
    return util;
}

auto Util::create_itemid() const -> model::ItemId { return {}; }

auto Util::mprisTrackid(model::ItemId id) const -> QString {
    static const auto dbus_path = QStringLiteral(APP_ID).replace('.', '/');
    // auto              provider  = id.provider();
    return rstd::into(std::format("/{}/TrackId/{}", dbus_path, id.id()));
}

auto Util::create_route_msg(QVariantMap props) const -> model::RouteMsg {
    model::RouteMsg msg;
    msg.url   = (props.value("url").toUrl());
    msg.props = (props.value("props").toMap());
    return msg;
}

auto Util::image_url(model::ItemId id, enums::ImageType image_type) const -> QUrl {
    return rstd::into(std::format("image://qcm/{}/{}/{}", id.type(), id.id(), image_type));
}

auto Util::image_url(const QString& url) const -> QUrl {
    return rstd::into(std::format("image://qcm/{}", url));
}

auto Util::audio_url(model::ItemId id) const -> QUrl {
    return App::instance()->backend()->audio_url(id);
}

auto Util::collect_ids(QAbstractItemModel* model) const -> std::vector<model::ItemId> {
    std::vector<model::ItemId> out;
    auto                       roleNames = model->roleNames();
    int                        id_role { -1 };
    int                        can_role { -1 };
    QHashIterator              i { roleNames };
    while (i.hasNext()) {
        i.next();
        const auto& v = i.value();
        if (v == "itemId") {
            id_role = i.key();
        } else if (v == "canPlay") {
            can_role = i.key();
        }
    }
    if (id_role != -1) {
        for (int i = 0; i < model->rowCount(); i++) {
            auto idx = model->index(i, 0);
            if (can_role != -1) {
                auto can = model->data(idx, can_role).toBool();
                if (! can) continue;
            }
            auto id_v = model->data(idx, id_role);
            if (auto id_p = get_if<model::ItemId>(&id_v)) {
                out.emplace_back(*id_p);
            } else {
                _assert_(false);
            }
        }
    }
    return out;
}

int Util::dyn_card_width(qint32 containerWidth, qint32 spacing) const {
    return std::max<qint32>(160, containerWidth / 6.0 - spacing);
}

QUrl Util::special_route_url(enums::SpecialRoute r) const {
    using SR = enums::SpecialRoute;
    switch (r) {
    case SR::SRSetting: return u"qrc:/Qcm/App/qml/page/setting/SettingPage.qml"_s;
    case SR::SRAbout: return u"qrc:/Qcm/App/qml/page/AboutPage.qml"_s;
    case SR::SRStatus: return u"qrc:/Qcm/App/qml/page/StatusPage.qml"_s;
    case SR::SRSearch: return u"qrc:/Qcm/App/qml/page/SearchPage.qml"_s;
    default: return {};
    }
}
model::RouteMsg Util::route_msg(enums::SpecialRoute r) const {
    model::RouteMsg msg;
    msg.url = (special_route_url(r));
    return msg;
}

void Util::print(const QJSValue& val) const {
    if (val.isObject()) {
        QJsonDocument    jdoc;
        QJsonObject      j;
        QJSValueIterator it(val);
        while (it.hasNext()) {
            it.next();
            j[it.name()] = it.value().toString();
        }
        jdoc.setObject(j);
        DEBUG_LOG("print: {}", jdoc.toJson(QJsonDocument::JsonFormat::Indented).toStdString());
    } else if (auto var = val.toVariant(); var.isValid()) {
        auto meta = var.metaType();
        DEBUG_LOG(R"(print
metaType: {}
metaId: {}
isNull: {}
json: {}
)",
                  meta.name(),
                  meta.id(),
                  var.toJsonDocument().toJson(QJsonDocument::JsonFormat::Indented).toStdString(),
                  var.isNull());
    } else {
        DEBUG_LOG("print: {}", val.toString());
    }
}

auto Util::artistId(QString id) const -> model::ItemId {
    return { enums::ItemType::ItemArtist, id.toLongLong() };
}

QString Util::joinName(const QJSValue& v, const QString& sp) const {
    auto to_name = [](const QJSValue& v) {
        if (v.hasProperty("name")) {
            return v.property("name").toString();
        }
        return v.toString();
    };

    QStringList list;

    if (v.isArray()) {
        const int length = v.property("length").toInt();
        for (int i = 0; i < length; ++i) {
            auto a = v.property(i);
            list.push_back(to_name(a));
        }
        return list.join(sp);
    } else if (v.isQObject()) {
        return to_name(v);
    } else if (v.isObject()) {
        auto var = v.toVariant();
        if (var.canConvert<QVariantList>()) {
            for (auto& a : var.toList()) {
                auto js = App::instance()->engine()->toScriptValue(a);
                list.push_back(to_name(js));
            }
            return list.join(sp);
        }
    } else if (v.isNull() || v.isUndefined()) {
        return {};
    }
    log::error("{}", v.toString());
    return {};
}

QString Util::formatDateTime(const QJSValue& v, const QString& format) const {
    do {
        if (v.isDate()) {
            return v.toDateTime().toString(format);
        } else if (v.isQObject()) {
            log::error("{}", v.toString());
            break;
        } else if (v.isNumber()) {
            auto d = QDateTime::fromMSecsSinceEpoch(v.toNumber() / 1e3);
            return d.toString(format);
        } else if (v.isObject()) {
            auto var = v.toVariant();
            if (auto t = get_if<google::protobuf::Timestamp>(&var)) {
                return t->toDateTime().toString(format);
            }
            log::error("{}", v.toString());
            break;
        } else {
            log::error("{}", v.toString());
        }
    } while (0);
    return {};
}
} // namespace qcm::qml

namespace qcm
{

auto gen_prefix(std::string_view in) -> std::string {
    return helper::to_upper(in.size() >= 2 ? in.substr(0, 2) : "no"sv);
}

inline std::string gen_file_name(std::string_view uniq) {
    return crypto::digest(crypto::md5(), convert_from<std::vector<byte>>(uniq))
        .map(crypto::hex::encode_up)
        .map(convert_from<std::string, crypto::bytes_view>)
        .unwrap();
}

} // namespace qcm

#include <Qcm/qml/moc_qml_util.cpp>