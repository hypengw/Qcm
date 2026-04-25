module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/query/artist_query.moc"
#endif

export module qcm:query.artist;
export import :query.query;
export import :model.list_models;
export import qextra;

namespace qcm
{

export class ArtistsQuery : public QueryList,
                            public QueryExtra<model::ArtistListModel, ArtistsQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<qcm::msg::filter::ArtistFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)
    Q_PROPERTY(QList<qcm::msg::filter::FilterLogic> filterLogics READ filterLogics WRITE
                   setFilterLogics NOTIFY filterLogicsChanged FINAL)

public:
    ArtistsQuery(QObject* parent = nullptr);

    auto filters() const -> const QList<msg::filter::ArtistFilter>&;
    void setFilters(const QList<msg::filter::ArtistFilter>&);

    auto filterLogics() const -> const QList<msg::filter::FilterLogic>&;
    void setFilterLogics(const QList<msg::filter::FilterLogic>&);

    void reload() override;
    void fetchMore(qint32) override;

    Q_SIGNAL void filtersChanged();
    Q_SIGNAL void filterLogicsChanged();

private:
    QList<msg::filter::ArtistFilter> m_filters;
    QList<msg::filter::FilterLogic>  m_filter_logics;
};

export class AlbumArtistsQuery : public QueryList,
                                 public QueryExtra<model::ArtistListModel, AlbumArtistsQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<qcm::msg::filter::ArtistFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)
    Q_PROPERTY(QList<qcm::msg::filter::FilterLogic> filterLogics READ filterLogics WRITE
                   setFilterLogics NOTIFY filterLogicsChanged FINAL)

public:
    AlbumArtistsQuery(QObject* parent = nullptr);
    auto filters() const -> const QList<msg::filter::ArtistFilter>&;
    void setFilters(const QList<msg::filter::ArtistFilter>&);

    auto filterLogics() const -> const QList<msg::filter::FilterLogic>&;
    void setFilterLogics(const QList<msg::filter::FilterLogic>&);

    Q_SIGNAL void filtersChanged();
    Q_SIGNAL void filterLogicsChanged();

    void reload() override;
    void fetchMore(qint32) override;

private:
    QList<msg::filter::ArtistFilter> m_filters;
    QList<msg::filter::FilterLogic>  m_filter_logics;
};

export class ArtistQuery : public Query, public QueryExtra<model::ArtistStoreItem, ArtistQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    ArtistQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

export class ArtistAlbumQuery : public QueryList,
                                public QueryExtra<model::AlbumListModel, ArtistAlbumQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    ArtistAlbumQuery(QObject* parent = nullptr);
    void reload() override;
    void fetchMore(qint32) override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} // namespace qcm
