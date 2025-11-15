#pragma once

#include <QQmlEngine>

#include "kstore/qt/gadget_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{

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

class AddToMixQuery : public Query, public QueryExtra<msg::Rsp, DeleteMixQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    AddToMixQuery(QObject* parent = nullptr);
    void reload() override;
};

class RemoveFromMixQuery : public Query, public QueryExtra<msg::Rsp, DeleteMixQuery> {
    Q_OBJECT
    QML_ELEMENT
public:
    RemoveFromMixQuery(QObject* parent = nullptr);
    void reload() override;
};

} // namespace qcm
