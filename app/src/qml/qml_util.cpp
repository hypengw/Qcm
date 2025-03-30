#include "Qcm/qml/qml_util.hpp"

#include <QtCore/QJsonDocument>
#include <QtQml/QJSValueIterator>

#include "meta_model/qgadget_helper.hpp"
#include "qcm_interface/global.h"
#include "qcm_interface/plugin.h"
#include "Qcm/app.h"
#include "qcm_interface/path.h"
#include "crypto/crypto.h"
#include "core/asio/basic.h"
#include "Qcm/sql/cache_sql.h"

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

auto Util::create_page() const -> model::Page { return {}; }
auto Util::create_itemid() const -> model::ItemId { return {}; }

auto Util::mpris_trackid(model::ItemId id) const -> QString {
    static const auto dbus_path = QStringLiteral(APP_ID).replace('.', '/');
    auto              provider  = id.provider();
    auto              sid       = id.id();
    return QStringLiteral("/%1/TrackId/%2/%3")
        .arg(dbus_path)
        .arg(provider.isEmpty() ? u"unknown"_s : provider)
        .arg(sid.isEmpty() ? u"0"_s : sid);
}

auto Util::create_route_msg(QVariantMap props) const -> model::RouteMsg {
    model::RouteMsg msg;
    msg.set_url(props.value("url").toUrl());
    msg.set_props(props.value("props").toMap());
    return msg;
}
auto Util::create_playlist(const QJSValue& js) const -> model::Mix {
    return meta_model::toGadget<model::Mix>(js);
}
auto Util::create_album(const QJSValue& js) const -> model::Album {
    return meta_model::toGadget<model::Album>(js);
}
auto Util::create_song(const QJSValue& js) const -> model::Song {
    return meta_model::toGadget<model::Song>(js);
}
auto Util::create_artist(const QJSValue& js) const -> model::Artist {
    return meta_model::toGadget<model::Artist>(js);
}
auto Util::create_djradio(const QJSValue& js) const -> model::Radio {
    return meta_model::toGadget<model::Radio>(js);
}
auto Util::create_program(const QJSValue& js) const -> model::Program {
    return meta_model::toGadget<model::Program>(js);
}

auto Util::image_url(const QString& library_id, const QString& item_id,
                     const QString& image_id) const -> QUrl {
    return rstd::into(fmt::format(
        "image://qcm/{}/{}/{}", library_id, item_id, image_id.isEmpty() ? "_" : image_id));
}
QUrl Util::image_cache_of(const QString& provider, const QUrl& url, QSize reqSize) const {
    auto out = qcm::image_uniq_hash(provider, url, reqSize).transform([](std::string_view id) {
        auto            p = qcm::cache_path_of(id);
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) {
            return QUrl::fromLocalFile(QString::fromStdString(p.native()));
        } else {
            return QUrl {};
        }
    });
    if (out) return out.value();
    return {};
}

QUrl Util::media_cache_of(const QString& id_) const {
    auto id        = convert_from<std::string>(id_);
    auto sql_cache = App::instance()->media_cache_sql();

    // for lru
    asio::co_spawn(sql_cache->get_executor(), sql_cache->get(id), asio::detached);

    auto path = qcm::media_cache_path_of(id);
    if (std::filesystem::exists(path)) {
        return QUrl::fromLocalFile(convert_from<QString>(path.native()));
    }
    return {};
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
    case SR::SRSetting: return u"qrc:/Qcm/App/qml/page/SettingsPage.qml"_s;
    case SR::SRAbout: return u"qrc:/Qcm/App/qml/page/AboutPage.qml"_s;
    case SR::SRStatus: return u"qrc:/Qcm/App/qml/page/StatusPage.qml"_s;
    case SR::SRSearch: return u"qrc:/Qcm/App/qml/page/SearchPage.qml"_s;
    default: return {};
    }
}
model::RouteMsg Util::route_msg(enums::SpecialRoute r) const {
    model::RouteMsg msg;
    msg.set_url(special_route_url(r));
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

QString Util::formatDateTime(const QJSValue& v, const QString& format) const {
    if (v.isDate()) {
        return v.toDateTime().toString(format);
    }
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

auto qcm::gen_image_cache_entry(const QString& provider, const QUrl& url, QSize reqSize)
    -> std::optional<std::filesystem::path> {
    return qcm::image_uniq_hash(provider, url, reqSize)
        .transform([](std::string_view id) -> std::filesystem::path {
            std::error_code ec;
            auto            path = cache_path_of(id);
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) {
                ERROR_LOG("{}", ec);
            }
            return path;
        });
}
auto qcm::image_uniq_hash(const QString& provider, const QUrl& url, QSize reqSize)
    -> std::optional<std::string> {
    return Global::instance()->plugin(provider).transform(
        [&url, reqSize](std::reference_wrapper<QcmPluginInterface> p) -> std::string {
            QcmPluginInterface* m;
            auto                uniq = p.get().uniq(url, reqSize);
            return gen_file_name(uniq.toStdString());
        });
}
auto qcm::song_uniq_hash(const model::ItemId& id, enums::AudioQuality quality) -> std::string {
    auto key = fmt::format("{}, quality: {}", id.toUrl().toString(), (i32)quality);
    return gen_file_name(key);
}

auto qcm::cache_path_of(std::string_view id) -> std::filesystem::path {
    auto cache_dir = cache_path() / "cache";
    auto file      = cache_dir / gen_prefix(id) / id;
    return file;
}
auto qcm::media_cache_path_of(std::string_view id) -> std::filesystem::path {
    auto media_cache_dir = cache_path() / "media";
    auto file            = media_cache_dir / gen_prefix(id) / id;
    return file;
}

#include <Qcm/qml/moc_qml_util.cpp>