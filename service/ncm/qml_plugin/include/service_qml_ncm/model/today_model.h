#pragma once

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "meta_model/qgadgetlistmodel.h"

namespace ncm::qml
{
struct TodayModelItem {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
};

class TodayModel : public meta_model::QGadgetListModel<TodayModelItem,
                                                       meta_model::QMetaListStore::VectorWithMap> {
    Q_OBJECT
public:
    TodayModel(QObject* parent)
        : meta_model::QGadgetListModel<TodayModelItem, meta_model::QMetaListStore::VectorWithMap>(
              parent) {
        convert(m_daily_song_id,
                ncm::model::SpecialId { std::string(ncm::model::SpecialId_DailySongRecommend) });
        insert(rowCount(),
               std::array { TodayModelItem { .id   = m_daily_song_id,
                                             .name = u"Daily recommended songs"_s } });
    }

    using ItemId = qcm::model::ItemId;

    auto dailySongId() -> const qcm::model::ItemId& { return m_daily_song_id; }
    auto dailySongItem() -> TodayModelItem& { return value_at(std::hash<ItemId>()(dailySongId())); }

    auto hash(const TodayModelItem& item) const noexcept -> usize override {
        return std::hash<qcm::model::ItemId>()(item.id);
    }

private:
    qcm::model::ItemId m_daily_song_id;
};

class TodayQuery : public qcm::QAsyncResultT<TodayModel> {
    Q_OBJECT
    QML_ELEMENT
public:
    TodayQuery(QObject* parent = nullptr): qcm::QAsyncResultT<TodayModel>(parent) {}
    virtual void reload() override {}
};

} // namespace ncm::qml