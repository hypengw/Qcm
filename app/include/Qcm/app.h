#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>

#include <iostream>
#include <vector>

#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>

#include "core/core.h"
#include "core/str_helper.h"
#include "core/log.h"
#include "asio_qt/qt_executor.h"

#include "ncm/client.h"

// #include "service_qml_ncm/model/user_account.h"
#include "mpris/mpris.h"
#include "mpris/mediaplayer2.h"
#include "media_cache/media_cache.h"
#include "qcm_interface/model/user_account.h"
#include "Qcm/player.h"
#include "Qcm/playlist.h"

#include "qcm_interface/global.h"

namespace qcm
{
namespace qml
{
class Util;
}
class CacheSql;
class CollectionSql;

void register_meta_type();
auto gen_image_cache_entry(const QString& provider, const QUrl& url,
                           QSize reqSize) -> std::optional<std::filesystem::path>;

auto cache_path_of(std::string_view id) -> std::filesystem::path;
auto media_cache_path_of(std::string_view id) -> std::filesystem::path;
auto image_id(const QString& provider, const QUrl& url,
              QSize reqSize) -> std::optional<std::string>;

class App : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(mpris::MediaPlayer2* mpris READ mpris CONSTANT FINAL)
    Q_PROPERTY(bool debug READ debug CONSTANT FINAL)
    Q_PROPERTY(Global* global READ global CONSTANT FINAL)
    Q_PROPERTY(Playlist* playlist READ playlist CONSTANT FINAL)

    friend class qml::Util;

public:
    using pool_executor_t = asio::thread_pool::executor_type;
    using qt_executor_t   = QtExecutor;

    enum ImageQuality
    {
        Img400px  = 400,
        Img800px  = 800,
        Img1200px = 1200,
        ImgAuto   = -1,
        ImgOrigin = -2,
    };
    Q_ENUMS(ImageQuality)

    enum ProxyType
    {
        PROXY_HTTP    = 0,
        PROXY_HTTPS2  = 3,
        PROXY_SOCKS4  = 4,
        PROXY_SOCKS5  = 5,
        PROXY_SOCKS4A = 6,
        PROXY_SOCKS5H = 7
    };
    Q_ENUMS(ProxyType)

    App(std::monostate);
    virtual ~App();
    static App* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // make qml prefer create
    App() = delete;

    void init();

    static auto instance() -> App*;
    auto        engine() const -> QQmlApplicationEngine*;
    auto        global() const -> Global*;
    auto        util() const -> qml::Util*;
    auto        playlist() const -> Playlist*;
    void        set_player_sender(Sender<Player::NotifyInfo>);
    auto        media_cache_sql() const -> rc<CacheSql>;
    auto        cache_sql() const -> rc<CacheSql>;

    mpris::MediaPlayer2* mpris() const { return m_mpris->mediaplayer2(); }

    bool debug() const;

    Q_INVOKABLE QString media_url(const QString& ori, const QString& id) const;
    Q_INVOKABLE QString md5(QString) const;

    Q_INVOKABLE bool    isItemId(const QJSValue&) const;
    Q_INVOKABLE QString itemIdPageUrl(const QJSValue&) const;

    Q_INVOKABLE qreal  devicePixelRatio() const;
    Q_INVOKABLE QSizeF image_size(QSizeF display, int quality, QQuickItem* = nullptr) const;
    Q_INVOKABLE QSizeF bound_image_size(QSizeF displaySize) const;

    Q_INVOKABLE void test();

Q_SIGNALS:
    void instanceStarted();
    void songLiked(model::ItemId, bool);
    void artistLiked(model::ItemId, bool);
    void albumLiked(model::ItemId, bool);
    void playlistLiked(model::ItemId, bool);
    void djradioLiked(model::ItemId, bool);
    void programLiked(model::ItemId, bool);
    void playlistCreated();
    void playlistDeleted();
    void playlistChanged();

public Q_SLOTS:
    void releaseResources(QQuickWindow*);
    void triggerCacheLimit();
    void setProxy(ProxyType, QString);
    void setVerifyCertificate(bool);
    void load_settings();
    void save_settings();

    void on_queue_songs(const std::vector<model::Song>&);
    void on_logout();
    void on_load_session(model::Session*);
    void on_switch_user(model::ItemId);

private:
    void load_plugins();
    void connect_actions();

    rc<Global>                  m_global;
    rc<qml::Util>               m_util;
    Playlist*                   m_playlist;
    up<mpris::Mpris>            m_mpris;
    rc<media_cache::MediaCache> m_media_cache;

    rc<CacheSql>      m_media_cache_sql;
    rc<CacheSql>      m_cache_sql;
    rc<CollectionSql> m_collect_sql;

    std::optional<Sender<Player::NotifyInfo>> m_player_sender;
    QPointer<QQuickWindow>                    m_main_win;
    up<QQmlApplicationEngine>                 m_qml_engine;
};
} // namespace qcm