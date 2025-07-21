#pragma once

#include "core/core.h"

#include <QtCore/QObject>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include <vector>

#include <asio/thread_pool.hpp>
#include <asio/steady_timer.hpp>

#include "core/qasio/qt_executor.h"

#include "Qcm/global.hpp"
#include "Qcm/model/empty_model.hpp"
#include "Qcm/model/item_id.hpp"
#include "player/notify.h"

namespace qcm
{
namespace qml
{
class Util;
}
namespace model
{
class IdQueue;
}
class CacheSql;
class ItemSql;
class PlayIdQueue;
class PlayQueue;
class Backend;
class ProviderStatusModel;
class ProviderMetaStatusModel;
class LibraryStatus;
class PageModel;
class AppStore;

class App : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QObject* mpris READ mpris CONSTANT FINAL)
    Q_PROPERTY(bool debug READ debug CONSTANT FINAL)
    Q_PROPERTY(qcm::Global* global READ global CONSTANT FINAL)
    Q_PROPERTY(qcm::PlayQueue* playqueue READ playqueue CONSTANT FINAL)
    Q_PROPERTY(qcm::model::EmptyModel* empty READ empty CONSTANT FINAL)
    Q_PROPERTY(
        qcm::ProviderMetaStatusModel* providerMetaStatus READ provider_meta_status CONSTANT FINAL)
    Q_PROPERTY(qcm::ProviderStatusModel* providerStatus READ provider_status CONSTANT FINAL)
    Q_PROPERTY(
        qcm::LibraryStatus* libraryStatus READ libraryStatus NOTIFY libraryStatusChanged FINAL)
    Q_PROPERTY(qcm::PageModel* pages READ pages CONSTANT FINAL)
    Q_PROPERTY(qcm::model::AppInfo info READ info CONSTANT FINAL)
    Q_PROPERTY(qcm::AppState* appState READ app_state CONSTANT FINAL)

    friend class qml::Util;

public:
    using pool_executor_t = asio::thread_pool::executor_type;
    using qt_executor_t   = QtExecutor;

    App(QStringView backend_exe, std::monostate);
    virtual ~App();
    static App* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // make qml prefer create
    App() = delete;

    void init();

    static auto instance() -> App*;
    auto        engine() const -> QQmlApplicationEngine*;
    auto        global() const -> Global*;
    auto        backend() const -> Backend*;
    auto        util() const -> qml::Util*;
    auto        playqueue() const -> PlayQueue*;
    auto        play_id_queue() const -> PlayIdQueue*;
    void        set_player_sender(player::Notifier);
    auto        empty() const -> model::EmptyModel*;
    auto        provider_meta_status() const -> ProviderMetaStatusModel*;
    auto        provider_status() const -> ProviderStatusModel*;
    auto        libraryStatus() const -> LibraryStatus*;
    auto        pages() const -> PageModel*;
    auto        store() const -> AppStore*;
    auto        info() const -> const model::AppInfo&;
    auto        app_state() const -> AppState*;
    void        switchPlayIdQueue();

    QObject* mpris() const;

    bool debug() const;

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
    Q_SIGNAL void libraryStatusChanged();

    Q_SLOT void releaseResources(QQuickWindow*);
    Q_SLOT void triggerCacheLimit();
    Q_SLOT void setProxy(enums::ProxyType, QString);
    Q_SLOT void setVerifyCertificate(bool);
    Q_SLOT void load_settings();
    Q_SLOT void save_settings();

    // process action
    Q_SLOT void on_play(model::ItemId songId, model::ItemId sourceId);
    Q_SLOT void on_queue_next(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void on_queue(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void on_switch(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void on_switch_queue(model::IdQueue*);
    Q_SLOT void on_logout();
    Q_SLOT void on_record(enums::RecordAction);
    Q_SLOT void on_route_by_id(const model::ItemId&, const QVariantMap&);

private:
    void load_plugins();
    void connect_actions();
    void register_meta_type();

    class Private;
    C_DECLARE_PRIVATE(App, d_ptr);
};
} // namespace qcm