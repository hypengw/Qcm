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

    Q_INVOKABLE static msg::filter::AlbumArtistIdFilter albumArtistIdFilter() noexcept;
    Q_INVOKABLE static msg::filter::ArtistIdFilter      artistIdFilter() noexcept;
    Q_INVOKABLE static msg::filter::TitleFilter         titleFilter() noexcept;
    Q_INVOKABLE static msg::filter::AlbumFilter         albumFilter() noexcept;
    Q_INVOKABLE static msg::filter::LastPlayedAtFilter  lastPlayedAtFilter() noexcept;
    Q_INVOKABLE static msg::filter::TypeStringFilter    typeStringFilter() noexcept;

    Q_INVOKABLE static model::ItemId   createItemid();
    Q_INVOKABLE static model::RouteMsg create_route_msg(QVariantMap);
    Q_INVOKABLE static model::RouteMsg routeMsg();

    Q_INVOKABLE static std::vector<model::ItemId> collect_ids(QAbstractItemModel* model);
    Q_INVOKABLE static int             dyn_card_width(qint32 containerWidth, qint32 spacing = 0);
    Q_INVOKABLE static void            print(const QJSValue&);
    Q_INVOKABLE static QUrl            special_route_url(enums::SpecialRoute);
    Q_INVOKABLE static model::RouteMsg route_msg(enums::SpecialRoute);

    Q_INVOKABLE static QUrl image_url(model::ItemId    id,
                                      enums::ImageType image_type = enums::ImageType::ImagePrimary);
    Q_INVOKABLE static QUrl image_url(const QString&);
    Q_INVOKABLE static QUrl audio_url(model::ItemId id);

    Q_INVOKABLE static model::ItemId albumArtistId(QString);
    Q_INVOKABLE static model::ItemId artistId(QString);
    Q_INVOKABLE static QString       mprisTrackid(model::ItemId);
    Q_INVOKABLE static QString       joinName(const QJSValue&, const QString& = "/");
    Q_INVOKABLE static QString       formatDateTime(const QJSValue&, const QString&);

    Q_INVOKABLE static QString prettyBytes(qint64 bytes, int maxFrac = 0);
    Q_INVOKABLE static QString formatDuration(qint64 msec, const QString& format = "mm:ss");
};
} // namespace qml

} // namespace qcm