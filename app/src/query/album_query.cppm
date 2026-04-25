module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#include "Qcm/query/album_query.moc"
#endif

export module qcm:query.album;
export import :query.query;
export import :model.list_models;

namespace qcm
{

export class AlbumsQuery : public QueryList, public QueryExtra<model::AlbumListModel, AlbumsQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<qcm::msg::filter::AlbumFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)
    Q_PROPERTY(qcm::msg::filter::FilterLogicGadget::FilterLogic groupLogic READ groupLogic WRITE
                   setGroupLogic NOTIFY groupLogicChanged FINAL)

public:
    AlbumsQuery(QObject* parent = nullptr);

    auto filters() const -> const QList<msg::filter::AlbumFilter>&;
    void setFilters(const QList<msg::filter::AlbumFilter>&);

    auto groupLogic() const -> msg::filter::FilterLogicGadget::FilterLogic;
    void setGroupLogic(msg::filter::FilterLogicGadget::FilterLogic);

    void reload() override;
    void fetchMore(qint32) override;

    Q_SIGNAL void filtersChanged();
    Q_SIGNAL void groupLogicChanged();

private:
    QList<msg::filter::AlbumFilter>           m_filters;
    msg::filter::FilterLogicGadget::FilterLogic m_group_logic {
        msg::filter::FilterLogicGadget::FilterLogic::FILTER_LOGIC_UNSPECIFIED
    };
};

export class AlbumQuery : public QueryList, public QueryExtra<model::AlbumSongListModel, AlbumQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    AlbumQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} 

