#pragma once

#include <QQmlEngine>

#include "kstore/qt/gadget_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{

class RemoteMixesQuery : public QueryList,
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

class MixesQuery : public QueryList, public QueryExtra<model::MixListModel, MixesQuery> {
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

class MixQuery : public Query, public QueryExtra<model::MixStoreItem, MixQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)

public:
    MixQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

class MixSongsQuery : public QueryList, public QueryExtra<model::MixSongListModel, MixSongsQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)

public:
    MixSongsQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

class CreateMixQuery : public Query, public QueryExtra<msg::CreateMixRsp, CreateMixQuery> {
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

class DeleteMixQuery : public Query, public QueryExtra<msg::Rsp, DeleteMixQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(std::vector<model::ItemId> ids READ ids WRITE setIds NOTIFY idsChanged FINAL)
public:
    DeleteMixQuery(QObject* parent = nullptr);
    void reload() override;

    auto ids() const -> std::vector<model::ItemId>;
    void setIds(const std::vector<model::ItemId>&);

    Q_SIGNAL void idsChanged();

private:
    std::vector<model::ItemId> m_ids;
};

class MixManipulateQuery : public Query,
                           public QueryExtra<msg::MixManipulateRsp, MixManipulateQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId mixId READ mixId WRITE setMixId NOTIFY mixIdChanged FINAL)
    Q_PROPERTY(qcm::msg::model::MixManipulateOperGadget::MixManipulateOper oper READ oper WRITE
                   setOper NOTIFY operChanged FINAL)
    Q_PROPERTY(std::vector<qcm::model::ItemId> songIds READ songIds WRITE setSongIds NOTIFY
                   songIdsChanged FINAL)
    Q_PROPERTY(std::vector<qcm::model::ItemId> albumIds READ albumIds WRITE setAlbumIds NOTIFY
                   albumIdsChanged FINAL)
public:
    MixManipulateQuery(QObject* parent = nullptr);
    void reload() override;

    auto mixId() const -> model::ItemId;
    void setMixId(const model::ItemId&);

    auto songIds() const -> std::vector<model::ItemId>;
    void setSongIds(const std::vector<model::ItemId>&);
    auto albumIds() const -> std::vector<model::ItemId>;
    void setAlbumIds(const std::vector<model::ItemId>&);

    auto oper() const -> msg::model::MixManipulateOperGadget::MixManipulateOper;
    void setOper(msg::model::MixManipulateOperGadget::MixManipulateOper);

    Q_SIGNAL void mixIdChanged();
    Q_SIGNAL void songIdsChanged();
    Q_SIGNAL void albumIdsChanged();
    Q_SIGNAL void operChanged();

private:
    model::ItemId                                          m_mix_id;
    msg::model::MixManipulateOperGadget::MixManipulateOper m_oper;
    std::vector<model::ItemId>                             m_song_ids;
    std::vector<model::ItemId>                             m_album_ids;
};

} // namespace qcm
