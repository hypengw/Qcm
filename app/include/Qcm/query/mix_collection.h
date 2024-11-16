#pragma once

#include <ranges>
#include <QQmlEngine>

#include "asio_qt/qt_sql.h"
#include "asio_helper/detached_log.h"
#include "meta_model/qgadgetlistmodel.h"

#include "qcm_interface/global.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/async.inl"
#include "qcm_interface/sql/meta_sql.h"

#include "qcm_interface/query.h"
#include "Qcm/query/query_load.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/sql/collection_sql.h"
#include "Qcm/app.h"

namespace qcm::query
{
struct MixCollectionItem : public model::Mix {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QDateTime, subTime, subTime)
};

class MixCollection
    : public meta_model::QGadgetListModel<MixCollectionItem,
                                          meta_model::QMetaListStore::VectorWithMap> {
    Q_OBJECT
public:
    MixCollection(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<MixCollectionItem,
                                       meta_model::QMetaListStore::VectorWithMap>(parent),
          m_has_more(true) {}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);

    auto hash(const MixCollectionItem& t) const noexcept -> usize override {
        return std::hash<model::ItemId>()(t.id);
    }

private:
    bool m_has_more;
};

class MixCollectionQuery : public Query<MixCollection> {
    Q_OBJECT
    QML_ELEMENT
public:
    MixCollectionQuery(QObject* parent = nullptr): Query<MixCollection>(parent) {
        set_use_queue(true);
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
        connect(
            Notifier::instance(), &Notifier::itemChanged, this, &MixCollectionQuery::onItemChanged);
    }

public:
    Q_SLOT void onItemChanged(const model::ItemId& id) {
        auto hash = std::hash<model::ItemId>()(id);
        if (! this->tdata()->contains_hash(hash)) return;
        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        asio::co_spawn(
            ex,
            [self, id, hash] -> task<void> {
                auto out = co_await self->query_item(id);
                co_await asio::post(
                    asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
                if (self && out) {
                    auto idx = self->tdata()->idx_at(hash);
                    self->tdata()->update(idx, *out);
                }
            },
            helper::asio_detached_log_t {});
    }

    auto query_item(const model::ItemId& itemId) -> task<std::optional<MixCollectionItem>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0},
    collection.collectTime 
FROM playlist 
JOIN collection ON playlist.itemId = collection.itemId
WHERE playlist.itemId = :itemId
GROUP BY playlist.itemId;
)",
                                     model::Mix::sql().select));
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        }
        if (query.next()) {
            MixCollectionItem item;
            int               i = 0;
            query::load_query<model::Mix>(query, item, i);
            item.subTime = query.value(i++).toDateTime();
            co_return item;
        }
        co_return std::nullopt;
    }

    auto query_collect(const model::ItemId& userId, const QDateTime& time)
        -> task<std::vector<MixCollectionItem>> {
        auto                           sql = App::instance()->album_sql();
        std::vector<MixCollectionItem> items;
        co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0},
    collection.collectTime 
FROM playlist 
JOIN collection ON playlist.itemId = collection.itemId
WHERE collection.userId = :userId AND collection.type = "playlist" AND collection.collectTime > :time AND collection.removed = 0
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
            item.subTime = query.value(i++).toDateTime();
        }
        co_return items;
    }

    void reload() override {
        auto time = last();
        if (status() == Status::Uninitialized) {
            Action::instance()->sync_collection(enums::CollectionType::CTPlaylist);
        }
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

            auto deleted_vec = co_await sql->select_removed(userId, u"playlist"_s, time);
            std::unordered_set<model::ItemId> deleted(deleted_vec.begin(), deleted_vec.end());

            if (! missing.empty()) co_await SyncAPi::sync_items(missing);

            auto items = co_await self->query_collect(userId, time);

            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));

            if (self) {
                auto t = self->tdata();
                t->remove_if([&deleted](const auto& el) -> bool {
                    return deleted.contains(el.id);
                });

                auto last = time;
                for (auto& el : items) {
                    last = std::max<QDateTime>(last, el.subTime);
                    {
                        auto it = std::find_if(t->begin(), t->end(), [&el](const auto& sub) {
                            return sub.id == el.id;
                        });

                        if (it != t->end()) {
                            t->update(std::distance(t->begin(), it), el);
                            continue;
                        }
                    }
                    {
                        auto it = std::lower_bound(
                            t->begin(),
                            t->end(),
                            el,
                            [userId](const MixCollectionItem& el, const auto& val) {
                                auto left_is_user  = el.userId == userId;
                                auto right_is_user = val.userId == userId;
                                if (left_is_user && ! right_is_user) {
                                    return true;
                                } else if ((! left_is_user && right_is_user)) {
                                    return false;
                                } else {
                                    return el.subTime > val.subTime;
                                }
                            });
                        t->insert(std::distance(t->begin(), it), el);
                    }
                }
                self->setLast(last);
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}
};

} // namespace qcm::query
