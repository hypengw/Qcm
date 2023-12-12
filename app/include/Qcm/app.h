#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
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

#include "Qcm/model/user_account.h"
#include "mpris/mpris.h"
#include "mpris/mediaplayer2.h"
#include "media_cache/media_cache.h"

namespace qcm
{
class CacheSql;

class App : public QObject {
    Q_OBJECT

    Q_PROPERTY(mpris::MediaPlayer2* mpris READ mpris)
    Q_PROPERTY(bool debug READ debug)
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

    static App* self;

    App();
    virtual ~App();

    void init();

    static App*            instance();
    QQmlApplicationEngine* engine() const;

    ncm::Client     ncm_client() const;
    auto            get_executor() { return m_qt_ex; }
    pool_executor_t get_pool_executor() { return m_pool.get_executor(); }
    auto            get_cache_sql() { return m_cache_sql; }

    mpris::MediaPlayer2* mpris() const { return m_mpris->mediaplayer2(); }

    bool                    debug() const;
    Q_INVOKABLE QVariantMap info() const;

    Q_INVOKABLE QUrl    media_file(const QString& id) const;
    Q_INVOKABLE QString media_url(const QString& ori, const QString& id) const;
    Q_INVOKABLE QString md5(QString) const;
    Q_INVOKABLE model::ArtistId artistId(QString id) const;
    Q_INVOKABLE model::AlbumId albumId(QString id) const;
    Q_INVOKABLE QUrl           getImageCache(QString url, QSize reqSize) const;
    Q_INVOKABLE bool           isItemId(const QJSValue&) const;
    Q_INVOKABLE QString        itemIdPageUrl(const QJSValue&) const;

    Q_INVOKABLE model::Song song(const QJSValue& = {}) const;
    Q_INVOKABLE model::Album album(const QJSValue& = {}) const;
    Q_INVOKABLE model::Artist artist(const QJSValue& = {}) const;
    Q_INVOKABLE model::Djradio djradio(const QJSValue& = {}) const;
    Q_INVOKABLE model::Playlist playlist(const QJSValue& = {}) const;
    Q_INVOKABLE model::Program program(const QJSValue& = {}) const;

    Q_INVOKABLE qreal  devicePixelRadio() const;
    Q_INVOKABLE QSizeF image_size(QSizeF display, int quality, QQuickItem* = nullptr) const;

    Q_INVOKABLE void test();

signals:
    void instanceStarted();
    void errorOccurred(QString);
    void songLiked(model::SongId, bool);
    void artistLiked(model::ArtistId, bool);
    void albumLiked(model::AlbumId, bool);
    void playlistLiked(model::PlaylistId, bool);
    void djradioLiked(model::DjradioId, bool);
    void programLiked(model::ProgramId, bool);
    void playlistCreated();
    void playlistDeleted();
    void playlistChanged();

public slots:
    void releaseResources(QQuickWindow*);
    void loginPost(model::UserAccount*);
    void triggerCacheLimit();
    void setProxy(ProxyType, QString);

private:
    void              load_session();
    void              save_session();
    qt_executor_t     m_qt_ex;
    asio::thread_pool m_pool;

    rc<request::Session>        m_session;
    mutable ncm::Client         m_client;
    up<mpris::Mpris>            m_mpris;
    rc<media_cache::MediaCache> m_media_cache;

    rc<CacheSql> m_media_cache_sql;
    rc<CacheSql> m_cache_sql;

    QPointer<QQuickWindow>    m_main_win;
    up<QQmlApplicationEngine> m_qml_engine;
};
} // namespace qcm
