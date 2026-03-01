module;
#include <QtCore/QPointer>
#include <QtCore/QRunnable>
#include <QtQml/QQmlEngine>

#include <QSettings>
#include <QVariant>

#include "Qcm/action.moc.h"
#ifdef Q_MOC_RUN
#include "Qcm/action.moc"
#endif
Q_MOC_INCLUDE("src/model/id_queue.cppm")

export module qcm.action;
export import qcm.qml.enums;
export import qcm.util.global_static;
export import qcm.model.id_queue;
export import qcm.model.router_msg;

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
    void queue_next(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void queue(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void switch_songs(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});

    void playUrl(const QUrl& url, bool refresh = false);

    void record(enums::RecordAction);
    void playLog(qint32 action, model::ItemId songId, model::ItemId sourceId = {});

    void scheduleRenderJob(QRunnable* job, qint32 stage);
};

} // namespace qcm
module :private;

namespace qcm
{
auto Action::instance() -> Action* {
    static auto the =
        GlobalStatic::instance()->add<Action>("action", new Action(nullptr), [](Action* p) {
            delete p;
        });
    return the;
};

Action::Action(QObject* parent): QObject(parent) {}
Action::~Action() {}

Action* Action::create(QQmlEngine*, QJSEngine*) {
    auto act = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(act, QJSEngine::CppOwnership);
    return act;
}

} // namespace qcm

#include "Qcm/action.moc.cpp"