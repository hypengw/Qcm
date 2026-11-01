module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/query/mix_query.moc"
#endif

export module qcm:query.mix;
export import :query.query;
export import :model.list_models;
export import :msg;
export import :notifier;

namespace qcm
{

export class RemoteMixesQuery : public QueryList,
                                public QueryExtra<model::MixListModel, RemoteMixesQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<qcm::msg::filter::RemoteMixFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)
public:
    RemoteMixesQuery(QObject* parent = nullptr);
    auto filters() const -> const QList<msg::filter::RemoteMixFilter>&;
    void setFilters(const QList<msg::filter::RemoteMixFilter>&);

    void reload() override;
    void fetchMore(qint32) override;

    Q_SIGNAL void filtersChanged();

private:
    QList<msg::filter::RemoteMixFilter> m_filters;
};

export class MixesQuery : public QueryList, public QueryExtra<model::MixListModel, MixesQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<qcm::msg::filter::MixFilter> filters READ filters WRITE setFilters NOTIFY
                   filtersChanged FINAL)
public:
    MixesQuery(QObject* parent = nullptr);
    auto filters() const -> const QList<msg::filter::MixFilter>&;
    void setFilters(const QList<msg::filter::MixFilter>&);

    void reload() override;
    void fetchMore(qint32) override;

    Q_SIGNAL void filtersChanged();

private:
    QList<msg::filter::MixFilter> m_filters;
};

export class MixQuery : public Query, public QueryExtra<model::MixStoreItem, MixQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::MixId mixId READ mixId WRITE setMixId NOTIFY mixIdChanged)

public:
    MixQuery(QObject* parent = nullptr);
    void reload() override;

    auto mixId() const -> msg::id::MixId;
    void setMixId(msg::id::MixId);

    Q_SIGNAL void mixIdChanged();

private:
    msg::id::MixId m_mix_id;
};

export class MixSongsQuery : public QueryList,
                             public QueryExtra<model::MixSongListModel, MixSongsQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::MixId mixId READ mixId WRITE setMixId NOTIFY mixIdChanged)

public:
    MixSongsQuery(QObject* parent = nullptr);
    void        reload() override;
    void        fetchMore(qint32) override;
    Q_SLOT void refresh();

    auto mixId() const -> msg::id::MixId;
    void setMixId(msg::id::MixId);

    Q_SIGNAL void mixIdChanged();

private:
    void          load(bool force_refresh);
    msg::id::MixId m_mix_id;
};

export class CreateMixQuery : public Query, public QueryExtra<msg::CreateMixRsp, CreateMixQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
public:
    CreateMixQuery(QObject* parent = nullptr);
    void reload() override;

    auto name() const -> QString;
    void setName(const QString&);

    Q_SIGNAL void nameChanged();
    Q_SIGNAL void mixCreated(const QString& name);

private:
    QString m_name;
};

export class DeleteMixQuery : public Query, public QueryExtra<msg::Rsp, DeleteMixQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::MixIds ids READ ids WRITE setIds NOTIFY idsChanged FINAL)
public:
    DeleteMixQuery(QObject* parent = nullptr);
    void reload() override;

    auto ids() const -> msg::id::MixIds;
    void setIds(const msg::id::MixIds&);

    Q_SIGNAL void idsChanged();

private:
    msg::id::MixIds m_ids;
};

export class LinkMixQuery : public Query, public QueryExtra<msg::Rsp, LinkMixQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::MixIds ids READ ids WRITE setIds NOTIFY idsChanged FINAL)
public:
    LinkMixQuery(QObject* parent = nullptr);
    void reload() override;

    auto ids() const -> msg::id::MixIds;
    void setIds(const msg::id::MixIds&);

    Q_SIGNAL void idsChanged();

private:
    msg::id::MixIds m_ids;
};

export class MixManipulateQuery : public Query,
                                  public QueryExtra<msg::MixManipulateRsp, MixManipulateQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::msg::id::MixId mixId READ mixId WRITE setMixId NOTIFY mixIdChanged FINAL)
    Q_PROPERTY(qcm::msg::model::MixManipulateOperGadget::MixManipulateOper oper READ oper WRITE
                   setOper NOTIFY operChanged FINAL)
    Q_PROPERTY(
        qcm::msg::id::SongIds songIds READ songIds WRITE setSongIds NOTIFY songIdsChanged FINAL)
    Q_PROPERTY(qcm::msg::id::AlbumIds albumIds READ albumIds WRITE setAlbumIds NOTIFY
                   albumIdsChanged FINAL)
public:
    MixManipulateQuery(QObject* parent = nullptr);
    void reload() override;

    auto mixId() const -> msg::id::MixId;
    void setMixId(const msg::id::MixId&);

    auto songIds() const -> msg::id::SongIds;
    void setSongIds(const msg::id::SongIds&);
    auto albumIds() const -> msg::id::AlbumIds;
    void setAlbumIds(const msg::id::AlbumIds&);

    auto oper() const -> msg::model::MixManipulateOperGadget::MixManipulateOper;
    void setOper(msg::model::MixManipulateOperGadget::MixManipulateOper);

    Q_SIGNAL void mixIdChanged();
    Q_SIGNAL void songIdsChanged();
    Q_SIGNAL void albumIdsChanged();
    Q_SIGNAL void operChanged();

private:
    msg::id::MixId                                         m_mix_id;
    msg::model::MixManipulateOperGadget::MixManipulateOper m_oper;
    msg::id::SongIds                                       m_song_ids;
    msg::id::AlbumIds                                      m_album_ids;
};

} // namespace qcm
