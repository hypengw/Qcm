module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#include "Qcm/action.moc"
#endif
Q_MOC_INCLUDE("src/model/id_queue.cppm")

export module qcm:action;
export import :qml.enums;
export import :model.id_queue;
export import :model.router_msg;
export import qextra;



namespace qcm
{

export class Action : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Action(QObject* parent);
    ~Action();
    static auto    instance() -> Action*;
    static Action* create(QQmlEngine*, QJSEngine*);
    // make qml prefer create
    Action() = delete;

Q_SIGNALS:
    void route(const QVariant&);
    void routeItem(const model::ItemId& id, const QVariantMap& props = {});
    void routeMain(qint32 idx);

    void openDrawer();
    void openItemMenu(const model::ItemId& id, QObject* parent, const QVariantMap& props = {});
    void openPopup(const QVariant&);

    void toast(QString text, qint32 duration = 3000, enums::ToastFlags flags = {},
               QObject* action = nullptr);
    void collect(model::ItemId itemId, bool act = true);
    void togglePlaybar();

    void toggle();
    void next();
    void prev();

    void switch_queue(model::IdQueue*);

    void play(model::ItemId songId, model::ItemId sourceId = {});
    void queue_next(const cppstd::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void queue(const cppstd::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void switch_songs(const cppstd::vector<model::ItemId>& songIds, model::ItemId sourceId = {});

    void playUrl(const QUrl& url, bool refresh = false);

    void record(enums::RecordAction);
    void playLog(qint32 action, model::ItemId songId, model::ItemId sourceId = {});

    void scheduleRenderJob(QRunnable* job, qint32 stage);
};

} // namespace qcm
