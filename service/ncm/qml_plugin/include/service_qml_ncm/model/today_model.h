#pragma once

#include "meta_model/qgadgetlistmodel.h"

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "service_qml_ncm/model/recommend_songs.h"
#include "service_qml_ncm/model/fm_queue.h"

#include "qcm_interface/action.h"
#include "qcm_interface/meta_name.h"
#include "qcm_interface/query.h"

#include "ncm/api/recommend_songs.h"

namespace ncm::qml
{
struct TodayModelItem {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(qcm::model::ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
};

class TodayQuery;
class TodayModel : public meta_model::QGadgetListModel<TodayModelItem,
                                                       meta_model::QMetaListStore::VectorWithMap> {
    Q_OBJECT
public:
    TodayModel(TodayQuery* q, QObject* parent);

    using ItemId = qcm::model::ItemId;

    auto dailySongId() -> const qcm::model::ItemId& { return m_daily_song_id; }
    auto dailySongItem() -> TodayModelItem& { return value_at(std::hash<ItemId>()(dailySongId())); }

    auto fmId() -> const qcm::model::ItemId& { return m_fm_id; }
    auto fmItem() -> TodayModelItem& { return value_at(std::hash<ItemId>()(fmId())); }

    auto radarId() -> const qcm::model::ItemId& { return m_radar_id; }
    auto radarItem() -> TodayModelItem& { return value_at(std::hash<ItemId>()(radarId())); }

    void dataChanged(const model::ItemId& id) {
        auto idx = index(this->idx_at(std::hash<model::ItemId>()(id)));
        QAbstractItemModel::dataChanged(idx, idx);
    }

    auto hash(const TodayModelItem& item) const noexcept -> usize override {
        return std::hash<qcm::model::ItemId>()(item.id);
    }

    Q_INVOKABLE bool trigger(const model::ItemId& id);

private:
    TodayQuery*        m_query;
    qcm::model::ItemId m_daily_song_id;
    qcm::model::ItemId m_fm_id;
    qcm::model::ItemId m_radar_id;
};

class TodayQuery : public qcm::QAsyncResultT<TodayModel, NcmApiQueryBase> {
    Q_OBJECT
    QML_ELEMENT
public:
    TodayQuery(QObject* parent = nullptr)
        : qcm::QAsyncResultT<TodayModel, NcmApiQueryBase>(parent, this),
          m_rmd_song_query(new RecommendSongsQuery(this)),
          m_fm(new FmQueue(this)) {
        m_mix_detail = qcm::query::QueryBase::create(qcm::MixDetailQueryMetaName);
        m_mix_detail->setParent(this);

        connect(
            m_rmd_song_query, &qcm::QAsyncResult::finished, this, &TodayQuery::onRmdSongFinished);
        connect(
            m_fm, &qcm::model::IdQueue::currentIndexChanged, this, &TodayQuery::onFmCurrentChanged);
        connect(m_fm, &qcm::model::IdQueue::rowsInserted, this, &TodayQuery::onFmCurrentChanged);
        connect(m_mix_detail, &qcm::QAsyncResult::finished, this, &TodayQuery::onMixDetailFinished);
    }

    virtual void reload() override {
        auto self = helper::QWatcher { this };
        m_mix_detail->setProperty("itemId", QVariant::fromValue(tdata()->radarId()));
        m_mix_detail->request_reload();
        m_rmd_song_query->reload();
        m_fm->requestNext();
    }

    auto fmQueue() { return m_fm; }
    auto dailySongQuery() { return m_rmd_song_query; }

private:
    Q_SLOT void onFmCurrentChanged() {
        auto picUrl = m_fm->currentOrFirstExtra()->toString();
        if (! picUrl.isEmpty()) {
            auto& item = tdata()->fmItem();
            if (ycore::cmp_exchange(item.picUrl, picUrl)) {
                item.picUrl = picUrl;
                tdata()->dataChanged(item.id);
            }
        }
    }

    Q_SLOT void onRmdSongFinished() {
        auto& item = this->tdata()->dailySongItem();
        auto  data = m_rmd_song_query->tdata();
        if (data->rowCount()) {
            auto& s     = data->at(0);
            item.picUrl = s.coverUrl.isEmpty() ? s.album.picUrl : s.coverUrl;
            tdata()->dataChanged(item.id);
        }
    }

    Q_SLOT void onMixDetailFinished() {
        auto& item = this->tdata()->radarItem();
        auto  data = m_mix_detail->data().value<QObject*>();
        if (data) {
            item.picUrl = meta_model::readOnGadget(data->property("info"), "picUrl").toString();
            tdata()->dataChanged(item.id);
        }
    }

private:
    RecommendSongsQuery*   m_rmd_song_query;
    FmQueue*               m_fm;
    qcm::query::QueryBase* m_mix_detail;
};

} // namespace ncm::qml