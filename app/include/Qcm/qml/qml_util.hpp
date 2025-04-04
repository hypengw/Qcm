#pragma once

#include <QQmlEngine>
#include <QAbstractItemModel>

#include "Qcm/model/router_msg.hpp"
#include "Qcm/qml/enum.hpp"

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
    Q_INVOKABLE QString       mpris_trackid(model::ItemId) const;
    Q_INVOKABLE model::RouteMsg create_route_msg(QVariantMap) const;

    Q_INVOKABLE QUrl media_cache_of(const QString& id) const;

    Q_INVOKABLE std::vector<model::ItemId> collect_ids(QAbstractItemModel* model) const;
    Q_INVOKABLE int  dyn_card_width(qint32 containerWidth, qint32 spacing = 0) const;
    Q_INVOKABLE void print(const QJSValue&) const;
    Q_INVOKABLE QUrl special_route_url(enums::SpecialRoute) const;
    Q_INVOKABLE model::RouteMsg route_msg(enums::SpecialRoute) const;

    Q_INVOKABLE QUrl image_url(model::ItemId    id,
                               enums::ImageType image_type = enums::ImageType::ImagePrimary) const;
    Q_INVOKABLE QUrl audio_url(model::ItemId id) const;

    Q_INVOKABLE QString joinName(const QJSValue&, const QString& = "/") const;
    Q_INVOKABLE QString formatDateTime(const QJSValue&, const QString&) const;
};
} // namespace qml

} // namespace qcm