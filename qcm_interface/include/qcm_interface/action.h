#pragma once

#include <QPointer>
#include <QQmlEngine>
#include "qcm_interface/export.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/model/router_msg.h"
#include "qcm_interface/model/session.h"
#include "qcm_interface/model/query_model.h"

namespace qcm
{

class QCM_INTERFACE_API Action : public QObject {
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
    void load_session(model::Session* session);
    void open_drawer();
    void logout();
    void route(model::RouteMsg msg);
    void route_special(QVariant name_id);
    void popup_special(QVariant name_id);
    void popup_page(const QJSValue& url_or_comp, QVariantMap props, QVariantMap popup_props = {},
                    const QJSValue& callback = {});
    void switch_main_page(qint32 idx);
    void toast(QString text, qint32 duration = 4000, enums::ToastFlags flags = {},
               QObject* action = nullptr);
    void collect(model::ItemId itemId, bool act = true);

    void next();
    void prev();
    void play(const query::Song&);
    void queue(const std::vector<query::Song>&);
    void switch_to(const std::vector<query::Song>&);
    void play_by_id(model::ItemId songId, model::ItemId sourceId = {});
    void queue_ids(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void switch_ids(const std::vector<model::ItemId>& songIds, model::ItemId sourceId = {});
    void play(const QUrl& url, bool refresh = false);
    void sync_collection(enums::CollectionType);
    void record(enums::RecordAction);
    void playbackLog(enums::PlaybackState state, model::ItemId item, model::ItemId souce,
                     QVariantMap extra = {});
};

} // namespace qcm