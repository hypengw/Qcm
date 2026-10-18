module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/query/home_query.moc"
#endif

export module qcm:query.home;
export import :query.play;
import :model.id_queue;

namespace qcm
{
export class HomeRecommendationsQuery : public Query {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::model::ForYouListModel* forYou READ forYou CONSTANT)
    Q_PROPERTY(qcm::model::RadioQueueListModel* radios READ radios CONSTANT)
public:
    HomeRecommendationsQuery(QObject* parent = nullptr);
    void             reload() override;
    auto             forYou() -> model::ForYouListModel*;
    auto             radios() -> model::RadioQueueListModel*;
    Q_INVOKABLE void activate(const qcm::model::ForYouCard&, bool playButton = false);
    Q_INVOKABLE void activateRadio(const qcm::model::RadioQueue&);
    Q_INVOKABLE qcm::model::DynamicIdQueue* radioQueue(const qcm::model::RadioQueue&);

private:
    void                                         reconcileProviders();
    void                                         apply();
    void                                         invalidate();
    model::ForYouListModel                       m_for_you;
    model::RadioQueueListModel                   m_radios;
    PlayQuery                                    m_play;
    QtProtobuf::int64List                        m_provider_ids;
    QMap<qint64, quint64>                        m_epochs;
    QMap<qint64, msg::model::HomeRecommendation> m_snapshots;
    quint64                                      m_revision { 0 };
};
} // namespace qcm
