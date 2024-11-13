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

// #include "service_qml_ncm/model/user_account.h"

#ifndef NODEBUS
#    include "mpris/mpris.h"
#    include "mpris/mediaplayer2.h"
#endif

#include "media_cache/media_cache.h"
#include "qcm_interface/model/user_account.h"
#include "Qcm/player.h"
#include "Qcm/play_queue.h"

#include "qcm_interface/global.h"
#include "qcm_interface/model/empty_model.h"

namespace qcm
{
namespace qml
{
class Util;
}
class CacheSql;
class CollectionSql;
class ItemSql;

void register_meta_type();
auto gen_image_cache_entry(const QString& provider, const QUrl& url, QSize reqSize)
    -> std::optional<std::filesystem::path>;

auto cache_path_of(std::string_view id) -> std::filesystem::path;
auto media_cache_path_of(std::string_view id) -> std::filesystem::path;
auto image_uniq_hash(const QString& provider, const QUrl& url, QSize reqSize)
    -> std::optional<std::string>;
auto song_uniq_hash(const model::ItemId& id, enums::AudioQuality quality) -> std::string;

class App : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QObject* mpris READ mpris CONSTANT FINAL)
    Q_PROPERTY(bool debug READ debug CONSTANT FINAL)
    Q_PROPERTY(qcm::Global* global READ global CONSTANT FINAL)
    Q_PROPERTY(qcm::PlayQueue* playqueue READ playqueue CONSTANT FINAL)
    Q_PROPERTY(qcm::model::EmptyModel* empty READ empty CONSTANT FINAL)

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
    auto        playqueue() const -> PlayQueue*;
    auto        play_id_queue() const -> PlayIdQueue*;
    void        set_player_sender(Sender<Player::NotifyInfo>);
    auto        media_cache_sql() const -> rc<CacheSql>;
    auto        cache_sql() const -> rc<CacheSql>;
    auto        album_sql() const -> rc<ItemSql>;
    auto        collect_sql() const -> rc<CollectionSql>;
    auto        empty() const -> model::EmptyModel*;
    void        switchPlayIdQueue();

    QObject* mpris() const;

    bool debug() const;

    Q_INVOKABLE QString media_url(const QUrl& ori, const QString& id) const;
    Q_INVOKABLE QString md5(QString) const;

    Q_INVOKABLE bool    isItemId(const QJSValue&) const;
    Q_INVOKABLE QString itemIdPageUrl(const QJSValue&) const;

    Q_INVOKABLE qreal  devicePixelRatio() const;
    Q_INVOKABLE QSizeF image_size(QSizeF display, int quality, QQuickItem* = nullptr) const;
    Q_INVOKABLE QSizeF bound_image_size(QSizeF displaySize) const;

    Q_INVOKABLE QVariant import_path_list();
    Q_INVOKABLE void     test();

    Q_SIGNAL void instanceStarted();
    Q_SIGNAL void songLiked(model::ItemId, bool);
    Q_SIGNAL void artistLiked(model::ItemId, bool);
    Q_SIGNAL void albumLiked(model::ItemId, bool);
    Q_SIGNAL void playlistLiked(model::ItemId, bool);
    Q_SIGNAL void djradioLiked(model::ItemId, bool);
    Q_SIGNAL void programLiked(model::ItemId, bool);
    Q_SIGNAL void playlistCreated();
    Q_SIGNAL void playlistDeleted();
    Q_SIGNAL void playlistChanged();

    Q_SLOT void releaseResources(QQuickWindow*);
    Q_SLOT void triggerCacheLimit();
    Q_SLOT void setProxy(ProxyType, QString);
    Q_SLOT void setVerifyCertificate(bool);
    Q_SLOT void load_settings();
    Q_SLOT void save_settings();

    Q_SLOT void on_play_by_id(model::ItemId songId, model::ItemId sourceId);
    Q_SLOT void on_queue_ids(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void on_switch_ids(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void on_switch_queue(model::IdQueue*);
    Q_SLOT void on_logout();
    Q_SLOT void on_load_session(model::Session*);
    Q_SLOT void on_switch_user(model::ItemId);
    Q_SLOT void on_collect(model::ItemId, bool);
    Q_SLOT void on_sync_item(const model::ItemId& itemId, bool notify);
    Q_SLOT void on_sync_collecttion(enums::CollectionType);
    Q_SLOT void on_record(enums::RecordAction);
    Q_SLOT void on_play_song(const query::Song&);
    Q_SLOT void on_queue(const std::vector<query::Song>&);
    Q_SLOT void on_switch_to(const std::vector<query::Song>&);

private:
    void load_plugins();
    void connect_actions();

    rc<Global>    m_global;
    rc<qml::Util> m_util;

    PlayIdQueue*       m_play_id_queue;
    PlayQueue*         m_playqueu;
    model::EmptyModel* m_empty;
#ifndef NODEBUS
    up<mpris::Mpris> m_mpris;
#endif

    rc<media_cache::MediaCache> m_media_cache;

    rc<CacheSql>      m_media_cache_sql;
    rc<CacheSql>      m_cache_sql;
    rc<CollectionSql> m_collect_sql;
    rc<ItemSql>       m_item_sql;

    std::optional<Sender<Player::NotifyInfo>> m_player_sender;
    QPointer<QQuickWindow>                    m_main_win;
    up<QQmlApplicationEngine>                 m_qml_engine;
};
} // namespace qcm