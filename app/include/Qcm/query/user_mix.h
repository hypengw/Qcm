#pragma once

#include <ranges>
#include <QQmlEngine>

#include "qcm_interface/query.h"
#include "Qcm/query/query_load.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/sql/collection_sql.h"
#include "Qcm/app.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"
#include "meta_model/qgadgetlistmodel.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/async.inl"
#include "qcm_interface/sql/meta_sql.h"

namespace qcm::query
{
class UserMix : public meta_model::QGadgetListModel<model::Mix> {
    Q_OBJECT
public:
    UserMix(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<model::Mix>(parent), m_has_more(true) {}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);

private:
    bool m_has_more;
};

class UserMixQuery : public Query<UserMix> {
    Q_OBJECT
    QML_ELEMENT
public:
    UserMixQuery(QObject* parent = nullptr): Query<UserMix>(parent) {
        connect(Notifier::instance(),
                &Notifier::collected,
                this,
                [this](const model::ItemId& id, bool) {
                    if (id.type() == u"playlist") {
                        request_reload();
                    }
                });
    }

public:
    auto query_mix(const model::ItemId& userId, const QDateTime& time)
        -> task<std::vector<model::Mix>> {
        auto                    sql = App::instance()->item_sql();
        std::vector<model::Mix> items;
        co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0},
    collection.collectTime 
FROM playlist 
JOIN collection ON playlist.itemId = collection.itemId
WHERE collection.userId = :userId AND collection.type = "playlist" AND collection.removed = 0 AND playlist.userId = :userId
GROUP BY playlist.itemId
ORDER BY collection.collectTime DESC;
)",
                                     model::Mix::sql().select));
        query.bindValue(":userId", userId.toUrl());
        query.bindValue(":time", time);

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        }
        while (query.next()) {
            auto& item = items.emplace_back();
            int   i    = 0;
            query::load_query<model::Mix>(query, item, i);
        }
        co_return items;
    }

    void reload() override {
        auto time = last();
        set_status(Status::Querying);
        auto userId = Global::instance()->qsession()->user()->userId();

        auto self = helper::QWatcher { this };
        spawn([self, userId, time] -> asio::awaitable<void> {
            auto sql     = App::instance()->collect_sql();
            auto missing = co_await sql->select_missing(
                userId,
                "playlist",
                "playlist",
                db::range_to<std::set<std::string>>(
                    db::meta_prop_names(model::Mix::staticMetaObject)));
            if (! missing.empty()) co_await SyncAPi::sync_items(missing);
            auto items = co_await self->query_mix(userId, time);
            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
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
