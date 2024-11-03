#pragma once

#include <QQmlEngine>
#include <QAbstractItemModel>

#include "qcm_interface/model/page.h"
#include "qcm_interface/model/router_msg.h"
#include "qcm_interface/model/album.h"

namespace qcm
{

namespace qml
{

class Util : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    Util(std::monostate);
    ~Util() override;
    static Util* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

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
    Q_INVOKABLE QUrl           image_url(const QUrl&) const;
    Q_INVOKABLE QUrl image_cache_of(const QString& provider, const QUrl& url, QSize reqSize) const;
    Q_INVOKABLE QUrl media_cache_of(const QString& id) const;

    Q_INVOKABLE std::vector<model::ItemId> collect_ids(QAbstractItemModel* model) const;
    Q_INVOKABLE int  dynCardWidth(qint32 containerWidth, qint32 spacing = 0) const;
    Q_INVOKABLE void print(const QJSValue&) const;
};
} // namespace qml

} // namespace qcm