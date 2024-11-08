#pragma once

#include <QQmlEngine>

#include "Qcm/app.h"
#include "Qcm/query/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"
#include "meta_model/qgadgetlistmodel.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/async.inl"

namespace qcm::query
{
struct DjradioCollectionItem : public model::Radio {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QDateTime, subTime, subTime)
};

class DjradioCollection : public meta_model::QGadgetListModel<DjradioCollectionItem> {
    Q_OBJECT
public:
    DjradioCollection(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<DjradioCollectionItem>(parent), m_has_more(true) {}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);

private:
    bool m_has_more;
};

class DjradioCollectionQuery : public Query<DjradioCollection> {
    Q_OBJECT
    QML_ELEMENT
public:
    DjradioCollectionQuery(QObject* parent = nullptr): Query<DjradioCollection>(parent) {}

public:
    void reload() override {
        if (status() == Status::Uninitialized) {
            Action::instance()->sync_collection(enums::CollectionType::CTDjradio);
        }
        set_status(Status::Querying);
        auto userId = Global::instance()->qsession()->user()->userId();

        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        spawn(ex, [self, userId] -> asio::awaitable<void> {
            auto                               sql = App::instance()->album_sql();
            std::vector<DjradioCollectionItem> items;
            co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
            {
                auto query = sql->con()->query();
                query.prepare(uR"(
SELECT 
    %1,
    collection.collectTime
FROM djradio
JOIN collection ON djradio.itemId = collection.itemId
WHERE collection.userId = :userId
GROUP BY djradio.itemId
ORDER BY collection.collectTime DESC;
)"_s.arg(model::Radio::sql().select));
                query.bindValue(":userId", userId.toUrl());

                if (! query.exec()) {
                    ERROR_LOG("{}", query.lastError().text());
                }
                while (query.next()) {
                    auto& item = items.emplace_back();
                    int   i    = 0;
                    query::load_query(item, query, i);
                    item.subTime = query.value(i++).toDateTime();
                }
            }
            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));

            if (self) {
                self->tdata()->resetModel(items);
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}
};

} // namespace qcm::query
