#include "service_qml_ncm/model/today_model.h"

namespace ncm::qml
{

TodayModel::TodayModel(TodayQuery* q, QObject* parent)
    : meta_model::QGadgetListModel<TodayModelItem, meta_model::QMetaListStore::VectorWithMap>(
          parent),
      m_query(q) {
    convert(m_daily_song_id, ncm::model::SpecialId { ncm::model::SpecialId_DailySongRecommend });
    convert(m_fm_id, ncm::model::SpecialId { ncm::model::SpecialId_UserFM });
    convert(m_radar_id, ncm::model::PlaylistId { ncm::model::RadarId_Private });
    insert(rowCount(),
           std::array { TodayModelItem { .id = m_daily_song_id, .name = u"daily songs"_s },
                        TodayModelItem { .id = m_fm_id, .name = u"private radio"_s },
                        TodayModelItem { .id = m_radar_id, .name = u"private radar mix"_s } });
}

bool TodayModel::trigger(const model::ItemId& id) {
    do {
        if (id == m_daily_song_id) {
            qcm::model::RouteMsg r {};
            QVariantMap          props;
            props["query"] = QVariant::fromValue(m_query->dailySongQuery());
            r.set_url(u"qrc:/Qcm/Service/Ncm/qml/page/DailySongPage.qml"_s);
            r.set_props(props);
            qcm::Action::instance()->route(r);
            break;
        } else if (id == m_fm_id) {
            qcm::Action::instance()->switch_queue(m_query->fmQueue());
            break;
        } else if (id == m_radar_id) {
            qcm::Action::instance()->route_by_id(m_radar_id);
            break;
        }
        return false;
    } while (false);
    return true;
}

} // namespace ncm::qml