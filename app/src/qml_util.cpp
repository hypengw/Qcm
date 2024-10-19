#include "Qcm/qml_util.h"
#include "meta_model/qgadget_helper.h"
#include "qcm_interface/global.h"
#include "qcm_interface/plugin.h"
#include "Qcm/app.h"
#include "qcm_interface/path.h"
#include "crypto/crypto.h"
#include "asio_helper/basic.h"
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
    static const auto dbus_path = QString(APP_ID).replace('.', '/');
    auto              provider  = id.provider();
    auto              sid       = id.id();
    return QString("/%1/TrackId/%2/%3")
        .arg(dbus_path)
        .arg(provider.isEmpty() ? u"unknown"_qs : provider)
        .arg(sid.isEmpty() ? u"0"_qs : sid);
}

auto Util::create_route_msg(QVariantMap props) const -> model::RouteMsg {
    model::RouteMsg msg;
    msg.set_url(props.value("url").toUrl());
    msg.set_props(props.value("props").toMap());
    return msg;
}
auto Util::create_playlist(const QJSValue& js) const -> model::Playlist {
    return meta_model::toGadget<model::Playlist>(js);
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
auto Util::create_djradio(const QJSValue& js) const -> model::Djradio {
    return meta_model::toGadget<model::Djradio>(js);
}
auto Util::create_program(const QJSValue& js) const -> model::Program {
    return meta_model::toGadget<model::Program>(js);
}

auto Util::image_url(const QUrl& in) const -> QUrl {
    if (in.scheme() == "image") return in;

    auto provider = Global::instance()->qsession()->provider();
    if (provider.isEmpty()) {
        return {};
    }

    return image_provider_url(in, provider);
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
    auto sql_cache = App::instance()->m_media_cache_sql;

    // for lru
    asio::co_spawn(sql_cache->get_executor(), sql_cache->get(id), asio::detached);

    auto path = qcm::media_cache_path_of(id);
    if (std::filesystem::exists(path)) {
        return QUrl::fromLocalFile(convert_from<QString>(path.native()));
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
    return UNWRAP(crypto::digest(crypto::md5(), convert_from<std::vector<byte>>(uniq))
                      .map(crypto::hex::encode_up)
                      .map(convert_from<std::string, crypto::bytes_view>));
}

} // namespace qcm


auto qcm::gen_image_cache_entry(const QString& provider, const QUrl& url,
                                QSize reqSize) -> std::optional<std::filesystem::path> {
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
auto qcm::image_uniq_hash(const QString& provider, const QUrl& url,
                          QSize reqSize) -> std::optional<std::string> {
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

