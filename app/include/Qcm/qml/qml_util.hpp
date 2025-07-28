#pragma once

#include <QQmlEngine>
#include <QAbstractItemModel>

#include "Qcm/model/router_msg.hpp"
#include "Qcm/model/item_id.hpp"
#include "Qcm/qml/enum.hpp"
#include "Qcm/message/filter.qpb.h"

namespace qcm
{

namespace qml
{

class Util : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(UtilCpp)
public:
    Util();
    ~Util() override;

    Q_INVOKABLE msg::filter::AlbumArtistIdFilter albumArtistIdFilter() const;
    Q_INVOKABLE msg::filter::ArtistIdFilter artistIdFilter() const;
    Q_INVOKABLE model::ItemId createItemid() const;
    Q_INVOKABLE model::RouteMsg create_route_msg(QVariantMap) const;
    Q_INVOKABLE model::RouteMsg routeMsg() const;

    Q_INVOKABLE std::vector<model::ItemId> collect_ids(QAbstractItemModel* model) const;
    Q_INVOKABLE int  dyn_card_width(qint32 containerWidth, qint32 spacing = 0) const;
    Q_INVOKABLE void print(const QJSValue&) const;
    Q_INVOKABLE QUrl special_route_url(enums::SpecialRoute) const;
    Q_INVOKABLE model::RouteMsg route_msg(enums::SpecialRoute) const;

    Q_INVOKABLE QUrl image_url(model::ItemId    id,
                               enums::ImageType image_type = enums::ImageType::ImagePrimary) const;
    Q_INVOKABLE QUrl image_url(const QString&) const;
    Q_INVOKABLE QUrl audio_url(model::ItemId id) const;

    Q_INVOKABLE model::ItemId artistId(QString) const;
    Q_INVOKABLE QString       mprisTrackid(model::ItemId) const;
    Q_INVOKABLE QString       joinName(const QJSValue&, const QString& = "/") const;
    Q_INVOKABLE QString       formatDateTime(const QJSValue&, const QString&) const;
};
} // namespace qml

} // namespace qcm