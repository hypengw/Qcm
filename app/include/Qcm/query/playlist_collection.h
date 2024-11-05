#pragma once

#include <QQmlEngine>

#include "Qcm/query/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/sql/collection_sql.h"
#include "Qcm/app.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"
#include "meta_model/qgadgetlistmodel.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/async.inl"

namespace qcm::query
{
struct PlaylistCollectionItem : public model::Playlist {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QDateTime, subTime, subTime)
};

class PlaylistCollection : public meta_model::QGadgetListModel<PlaylistCollectionItem> {
    Q_OBJECT
public:
    PlaylistCollection(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<PlaylistCollectionItem>(parent), m_has_more(true) {}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);

private:
    bool m_has_more;
};

class PlaylistCollectionQuery : public Query<PlaylistCollection> {
    Q_OBJECT
    QML_ELEMENT
public:
    PlaylistCollectionQuery(QObject* parent = nullptr): Query<PlaylistCollection>(parent) {
        connect(Notifier::instance(),
                &Notifier::collected,
                this,
                [this](const model::ItemId& id, bool) {
                    if (id.type() == u"playlist") {
                        request_reload();
                    }
                });
        connect(Notifier::instance(),
                &Notifier::collection_synced,
                this,
                [this](enums::CollectionType type) {
                    if (type == enums::CollectionType::CTPlaylist) {
                        request_reload();
                    }
                });
    }

public:
    auto select_sync(const model::ItemId& user_id) -> task<std::vector<model::ItemId>> {
        auto                       sql = App::instance()->album_sql();
        std::vector<model::ItemId> ids;
        co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
        auto query = sql->con()->query();
        query.prepare(uR"(
SELECT 
    itemId
FROM collection
WHERE userId = :userId AND type = "playlist" AND itemId NOT IN (SELECT itemId FROM playlist);
)"_s);
        query.bindValue(":userId", user_id.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        }
        while (query.next()) {
            auto& item = ids.emplace_back();
            ids.emplace_back(query.value(0).toUrl());
        }
        co_return ids;
    }
    auto query_collect(const model::ItemId& userId,
                       const QDateTime&     time) -> task<std::vector<PlaylistCollectionItem>> {
        auto                                sql = App::instance()->album_sql();
        std::vector<PlaylistCollectionItem> items;
        co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
        auto query = sql->con()->query();
        query.prepare(uR"(
SELECT 
    %1,
    collection.collectTime 
FROM playlist 
JOIN collection ON playlist.itemId = collection.itemId
WHERE collection.userId = :userId AND collection.type = "playlist" AND collection.collectTime > :time
GROUP BY playlist.itemId
ORDER BY collection.collectTime DESC;
)"_s.arg(model::Playlist::Select));
        query.bindValue(":userId", userId.toUrl());
        query.bindValue(":time", time);

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        }
        while (query.next()) {
            auto& item = items.emplace_back();
            int   i    = 0;
            query::load_query(item, query, i);
            item.subTime = query.value(i++).toDateTime();
        }
        co_return items;
    }

    void reload() override {
        auto time = record();
        if (status() == Status::Uninitialized) {
            Action::instance()->sync_collection(enums::CollectionType::CTPlaylist);
            time = QDateTime::fromSecsSinceEpoch(0);
        }
        set_status(Status::Querying);
        auto userId = Global::instance()->qsession()->user()->userId();

        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        spawn(ex, [self, userId, time] -> asio::awaitable<void> {
            auto sql  = App::instance()->collect_sql();
            auto sync = co_await self->select_sync(userId);

            auto deleted_vec = co_await sql->select_removed(userId, u"playlist"_s, time);
            std::unordered_set<model::ItemId> deleted(deleted_vec.begin(), deleted_vec.end());

            if (! sync.empty()) co_await SyncAPi::sync_items(sync);

            auto items = co_await self->query_collect(userId, time);

            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));

            if (self) {
                auto t = self->tdata();
                t->remove_if([&deleted](const auto& el) -> bool {
                    return deleted.contains(el.id);
                });

                for (auto& el : items) {
                    auto it = std::lower_bound(
                        t->begin(), t->end(), el, [userId](const auto& el, const auto& val) {
                            auto left_is_user  = el.userId == userId;
                            auto right_is_user = val.userId == userId;
                            if (left_is_user && ! right_is_user) {
                                return true;
                            } else if (! left_is_user && right_is_user) {
                                return false;
                            } else {
                                return el.subTime > val.subTime;
                            }
                        });
                    t->insert(std::distance(t->begin(), it), el);
                }
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}
};

} // namespace qcm::query
