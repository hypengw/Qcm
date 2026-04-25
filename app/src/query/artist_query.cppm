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
    Q_PROPERTY(qcm::msg::filter::FilterLogicGadget::FilterLogic groupLogic READ groupLogic WRITE
                   setGroupLogic NOTIFY groupLogicChanged FINAL)

public:
    ArtistsQuery(QObject* parent = nullptr);

    auto filters() const -> const QList<msg::filter::ArtistFilter>&;
    void setFilters(const QList<msg::filter::ArtistFilter>&);

    auto groupLogic() const -> msg::filter::FilterLogicGadget::FilterLogic;
    void setGroupLogic(msg::filter::FilterLogicGadget::FilterLogic);

    void reload() override;
    void fetchMore(qint32) override;

    Q_SIGNAL void filtersChanged();
    Q_SIGNAL void groupLogicChanged();

private:
    QList<msg::filter::ArtistFilter>            m_filters;
    msg::filter::FilterLogicGadget::FilterLogic m_group_logic {
        msg::filter::FilterLogicGadget::FilterLogic::FILTER_LOGIC_UNSPECIFIED
    };
};

export class AlbumArtistsQuery : public QueryList,
                                 public QueryExtra<model::ArtistListModel, AlbumArtistsQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<qcm::msg::filter::ArtistFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)
    Q_PROPERTY(qcm::msg::filter::FilterLogicGadget::FilterLogic groupLogic READ groupLogic WRITE
                   setGroupLogic NOTIFY groupLogicChanged FINAL)

public:
    AlbumArtistsQuery(QObject* parent = nullptr);
    auto filters() const -> const QList<msg::filter::ArtistFilter>&;
    void setFilters(const QList<msg::filter::ArtistFilter>&);

    auto groupLogic() const -> msg::filter::FilterLogicGadget::FilterLogic;
    void setGroupLogic(msg::filter::FilterLogicGadget::FilterLogic);

    Q_SIGNAL void filtersChanged();
    Q_SIGNAL void groupLogicChanged();

    void reload() override;
    void fetchMore(qint32) override;

private:
    QList<msg::filter::ArtistFilter>            m_filters;
    msg::filter::FilterLogicGadget::FilterLogic m_group_logic {
        msg::filter::FilterLogicGadget::FilterLogic::FILTER_LOGIC_UNSPECIFIED
    };
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
