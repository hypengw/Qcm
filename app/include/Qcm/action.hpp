#pragma once

#include <QPointer>
#include <QQmlEngine>
#include "core/core.h"
#include "Qcm/qml/enum.hpp"
#include "Qcm/model/router_msg.hpp"
#include "Qcm/model/item_id.hpp"

Q_MOC_INCLUDE("Qcm/model/id_queue.hpp")
namespace qcm
{

namespace model
{
class IdQueue;
}
class Action : public QObject {
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
    void switch_user(model::ItemId userId);
    void open_drawer();
    void logout();
    void route(const model::RouteMsg& msg);
    void route_by_id(const model::ItemId& id, const QVariantMap& props = {});
    void route_special(QVariant name_id);
    void menu_by_id(const model::ItemId& id, QObject* parent, const QVariantMap& props = {});
    void popup_special(QVariant name_id);
    void popup_page(const QJSValue& url_or_comp, QVariantMap props, QVariantMap popup_props = {},
                    const QJSValue& callback = {});
    void switch_main_page(qint32 idx);
    void toast(QString text, qint32 duration = 3000, enums::ToastFlags flags = {},
               QObject* action = nullptr);
    void collect(model::ItemId itemId, bool act = true);
    void toggle_playbar();

    void toggle();
    void next();
    void prev();

    void switch_queue(model::IdQueue*);

    void play(model::ItemId songId, model::ItemId sourceId = {});
    void queue_next(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void queue(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void switch_songs(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});

    void play_url(const QUrl& url, bool refresh = false);
    void record(enums::RecordAction);
    void playback_log(enums::PlaybackState state, model::ItemId item, model::ItemId souce,
                      QVariantMap extra = {});
};

} // namespace qcm