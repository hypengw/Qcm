module;
#include <QtCore/QSizeF>
#ifdef Q_MOC_RUN
#    include "Qcm/app.moc"
#endif

#include "QExtra/macro_qt.hpp"
#include "Qcm/core/macro.h"

export module qcm:app;
import rstd;
export import :backend;
export import :status.provider;
export import :queue;
export import :qml;
export import :image_provider;
export import :model.app_info;
export import :model.empty_model;
export import :model.item_id;
export import :model.page_model;
export import :global;
import qextra;
import platform;

using rstd::boxed::Box;

namespace qcm
{
export class App : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(qcm::Backend* backend READ backend CONSTANT FINAL)
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
    App(Box<Backend> backend, QString activation_token);
    virtual ~App();
    static App* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static void register_converters();

    // make qml prefer create
    App() = delete;

    void init();

    static auto instance() -> App*;
    auto        engine() const -> QQmlApplicationEngine*;
    auto        global() const -> Global*;
    auto        backend() const -> Backend*;
    auto        util() const -> qml::Util*;
    auto        playqueue() const -> PlayQueue*;
    auto        empty() const -> model::EmptyModel*;
    auto        provider_meta_status() const -> ProviderMetaStatusModel*;
    auto        provider_status() const -> ProviderStatusModel*;
    auto        libraryStatus() const -> LibraryStatus*;
    auto        pages() const -> PageModel*;
    auto        store() const -> AppStore*;
    auto        info() const -> const model::AppInfo&;
    auto        app_state() const -> AppState*;

    bool debug() const;

    Q_INVOKABLE qreal       devicePixelRatio() const;
    Q_INVOKABLE QVariantMap itemRouteProperties(const model::ItemId&, const QVariantMap&) const;
    // Q_INVOKABLE QSizeF image_size(QSizeF display, int quality, QQuickItem* = nullptr) const;
    Q_INVOKABLE QSizeF bound_image_size(QSizeF displaySize) const;

    Q_INVOKABLE QVariant import_path_list();
    Q_INVOKABLE void     test();

    Q_SIGNAL void libraryStatusChanged();

    Q_SLOT void releaseResources(QQuickWindow*, const QJSValue&);
    Q_SLOT void triggerCacheLimit();

private:
    // process action
    Q_SLOT void onPlay(model::ItemId songId, model::ItemId sourceId);
    Q_SLOT void onQueueNext(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void onQueue(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void onSwitch(const std::vector<model::ItemId>& songIds, model::ItemId sourceId);
    Q_SLOT void onSwitchQueue(model::DynamicIdQueue*);
    Q_SLOT void onRouteItem(const model::ItemId&, const QVariantMap&);

    void load_plugins();
    void connect_components();
    void connect_actions();
    void register_meta_type();

    class Private;
    C_DECLARE_PRIVATE(App, d_ptr);
};
} // namespace qcm
