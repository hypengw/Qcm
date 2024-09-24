#pragma once

#include <QQmlEngine>
#include "qcm_interface/export.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/model/router_msg.h"

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
    void open_drawer();
    void logout();
    void route(model::RouteMsg msg);
    void route_special(QVariant name_id);
    void popup_special(QVariant name_id);
    void toast(QString text, qint32 duration = 4000, enums::ToastFlags flags = {},
               QObject* action = nullptr);

    void queue_songs(const std::vector<model::Song>&);
    void playbackLog(enums::PlaybackState state, model::ItemId item, model::ItemId souce,
                     QVariantMap extra = {});
};

} // namespace qcm