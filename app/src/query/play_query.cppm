module;
#include "Qcm/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/query/play_query.moc"
#endif

export module qcm:query.play;
export import :query.query;
export import :query.album;

namespace qcm
{

export class PlayQuery : public Query {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    PlayQuery(QObject* parent = nullptr);
    void reload() override;

    auto          itemId() const -> model::ItemId;
    void          setItemId(model::ItemId);
    Q_SIGNAL void itemIdChanged(model::ItemId);

private:
    model::ItemId m_item_id;
};

export class PlayAllQuery : public Query {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool asc READ asc WRITE setAsc NOTIFY ascChanged FINAL)
    Q_PROPERTY(bool albumAsc READ albumAsc WRITE setAlbumAsc NOTIFY albumAscChanged FINAL)
    Q_PROPERTY(qint32 sort READ sort WRITE setSort NOTIFY sortChanged FINAL)
    Q_PROPERTY(qint32 albumSort READ albumSort WRITE setAlbumSort NOTIFY albumSortChanged FINAL)
    Q_PROPERTY(QList<qcm::msg::filter::AlbumFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)

public:
    PlayAllQuery(QObject* parent = nullptr);
    void reload() override;

    auto sort() const noexcept -> qint32;
    void setSort(qint32);
    auto asc() const noexcept -> bool;
    void setAsc(bool);

    auto albumSort() const noexcept -> qint32;
    auto albumAsc() const noexcept -> bool;
    void setAlbumSort(qint32 v);
    void setAlbumAsc(bool v);

    auto filters() const -> const QList<msg::filter::AlbumFilter>&;
    void setFilters(const QList<msg::filter::AlbumFilter>&);

    Q_SIGNAL void sortChanged();
    Q_SIGNAL void ascChanged();
    Q_SIGNAL void albumSortChanged();
    Q_SIGNAL void albumAscChanged();
    Q_SIGNAL void filtersChanged();

private:
    qint32 m_sort;
    bool   m_asc;
    qint32 m_album_sort;
    bool   m_album_asc;

    QList<msg::filter::AlbumFilter> m_filters;
};

export class RadioQueuesQuery
    : public QueryList,
      public QueryExtra<model::RadioQueueListModel, RadioQueuesQuery> {
    Q_OBJECT
    QML_ELEMENT

public:
    RadioQueuesQuery(QObject* parent = nullptr);
    void reload() override;
};

export class QueueNextQuery : public Query,
                              public QueryExtra<QList<msg::model::Song>, QueueNextQuery> {
    Q_OBJECT

    Q_PROPERTY(qint64 queueId READ queueId WRITE setQueueId NOTIFY queueIdChanged FINAL)
public:
    QueueNextQuery(QObject* parent = nullptr);
    void reload() override;

    auto queueId() const -> qint64;
    void setQueueId(qint64);

    Q_SIGNAL void queueIdChanged();

private:
    qint64 m_queue_id { 0 };
};

} // namespace qcm
