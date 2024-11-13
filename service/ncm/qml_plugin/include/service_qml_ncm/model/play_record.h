#pragma once

#include <ranges>

#include <QQmlEngine>
#include <QAbstractListModel>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "service_qml_ncm/enum.h"
#include "ncm/api/play_record.h"

#include "meta_model/qgadgetlistmodel.h"

namespace ncm::qml
{
namespace model
{
struct PlayRecordItem {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QVariant, data, data)
};
} // namespace model
} // namespace ncm::qml

DEFINE_CONVERT(qcm::model::Mix, ncm::model::PlayRecordItem::Playlist) {
    convert(out.id, in.id);
    convert(out.picUrl, in.coverImgUrl);
    convert(out.name, in.name);
}

DEFINE_CONVERT(ncm::qml::model::PlayRecordItem, ncm::model::PlayRecordItem) {
    auto data =
        std::visit(overloaded { [](const ncm::model::Song& in) {
                                   return QVariant::fromValue(convert_from<qcm::model::Song>(in));
                               },
                                [](const ncm::model::Album& in) {
                                    return QVariant::fromValue(convert_from<qcm::model::Album>(in));
                                },
                                [](const ncm::model::PlayRecordItem::Playlist& in) {
                                    return QVariant::fromValue(convert_from<qcm::model::Mix>(in));
                                },
                                [](const ncm::model::DjradioB& in) {
                                    return QVariant::fromValue(convert_from<qcm::model::Radio>(in));
                                } },
                   in.data);
    out.data = data;
}
namespace ncm::qml
{
namespace model
{

class PlayRecord : public meta_model::QGadgetListModel<PlayRecordItem> {
    Q_OBJECT
public:
    PlayRecord(QObject* parent = nullptr): meta_model::QGadgetListModel<PlayRecordItem>(parent) {}
    using out_type = ncm::api_model::PlayRecord;

    void handle_output(const out_type& re, const auto&) {
        auto view = std::ranges::transform_view(
            re.data.list, [](const ncm::model::PlayRecordItem& el) -> PlayRecordItem {
                return convert_from<PlayRecordItem>(el);
            });
        resetModel();
        insert(0, view);
    }
signals:
    void fetchMoreReq(qint32);
};
} // namespace model

using PlayRecordQuerier_base = NcmApiQuery<ncm::api::PlayRecord, model::PlayRecord>;
class PlayRecordQuerier : public PlayRecordQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    PlayRecordQuerier(QObject* parent = nullptr): PlayRecordQuerier_base(parent) {}

    FORWARD_PROPERTY(enums::IdType, type, type)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void        fetch_more(qint32) override {}
    Q_SLOT void reset() { reload(); }
};

} // namespace ncm::qml
