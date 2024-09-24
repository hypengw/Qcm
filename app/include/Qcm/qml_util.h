#pragma once

#include <QQmlEngine>

#include "qcm_interface/model/page.h"
#include "qcm_interface/model/router_msg.h"

namespace qcm::qml
{

class Util : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    Q_INVOKABLE model::Page create_page() const;
    Q_INVOKABLE model::ItemId create_itemid() const;
    Q_INVOKABLE QString       mpris_trackid(model::ItemId) const;
    Q_INVOKABLE model::RouteMsg create_route_msg(QVariantMap) const;
    Q_INVOKABLE model::Playlist create_playlist(const QJSValue& = {}) const;
    Q_INVOKABLE model::Album create_album(const QJSValue& = {}) const;
    Q_INVOKABLE model::Song create_song(const QJSValue& = {}) const;
    Q_INVOKABLE model::Artist create_artist(const QJSValue& = {}) const;
    Q_INVOKABLE model::Djradio create_djradio(const QJSValue& = {}) const;
    Q_INVOKABLE model::Program create_program(const QJSValue& = {}) const;
};

} // namespace qcm::qml