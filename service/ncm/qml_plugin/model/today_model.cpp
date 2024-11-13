#include "service_qml_ncm/model/today_model.h"

namespace ncm::qml
{

TodayModel::TodayModel(TodayQuery* q, QObject* parent)
    : meta_model::QGadgetListModel<TodayModelItem, meta_model::QMetaListStore::VectorWithMap>(
          parent),
      m_query(q) {
    convert(m_daily_song_id,
            ncm::model::SpecialId { std::string(ncm::model::SpecialId_DailySongRecommend) });
    convert(m_daily_song_id, ncm::model::SpecialId { std::string(ncm::model::SpecialId_UserFM) });
    insert(rowCount(),
           std::array { TodayModelItem { .id = m_daily_song_id, .name = u"daily songs"_s },
                        TodayModelItem { .id = m_fm_id, .name = u"private radio"_s } });
}

bool TodayModel::trigger(const model::ItemId& id) {
    do {
        if (id == m_daily_song_id) {
            break;
        } else if (id == m_fm_id) {
            qcm::Action::instance()->switch_queue(m_query->fmQueue());
            break;
        }
        return false;
    } while (false);
    return true;
}

} // namespace ncm::qml