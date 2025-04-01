#pragma once

#include <QQmlEngine>
#include <QAbstractItemModel>

#include "qcm_interface/model/router_msg.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/enum.h"

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

    Q_INVOKABLE model::ItemId create_itemid() const;
    Q_INVOKABLE model::ItemId create_itemid(QString type, QString id) const;
    Q_INVOKABLE QString       mpris_trackid(model::ItemId) const;
    Q_INVOKABLE model::RouteMsg create_route_msg(QVariantMap) const;
    Q_INVOKABLE model::Mix create_playlist(const QJSValue& = {}) const;
    Q_INVOKABLE model::Album create_album(const QJSValue& = {}) const;
    Q_INVOKABLE model::Song create_song(const QJSValue& = {}) const;
    Q_INVOKABLE model::Artist create_artist(const QJSValue& = {}) const;
    Q_INVOKABLE model::Radio create_djradio(const QJSValue& = {}) const;
    Q_INVOKABLE model::Program create_program(const QJSValue& = {}) const;

    Q_INVOKABLE QUrl image_cache_of(const QString& provider, const QUrl& url, QSize reqSize) const;
    Q_INVOKABLE QUrl media_cache_of(const QString& id) const;

    Q_INVOKABLE std::vector<model::ItemId> collect_ids(QAbstractItemModel* model) const;
    Q_INVOKABLE int  dyn_card_width(qint32 containerWidth, qint32 spacing = 0) const;
    Q_INVOKABLE void print(const QJSValue&) const;
    Q_INVOKABLE QUrl special_route_url(enums::SpecialRoute) const;
    Q_INVOKABLE model::RouteMsg route_msg(enums::SpecialRoute) const;

    Q_INVOKABLE QUrl image_url(const QString& item_type, const QString& item_id,
                               const QString& image_type = "Primary") const;
    Q_INVOKABLE QUrl audio_url(const QString& item_type, const QString& item_id) const;

    Q_INVOKABLE QString joinName(const QJSValue&, const QString& = "/") const;
    Q_INVOKABLE QString formatDateTime(const QJSValue&, const QString&) const;
};
} // namespace qml

} // namespace qcm