#pragma once

#include <QQmlEngine>

#include "asio_qt/qt_sql.h"
#include "meta_model/qgadgetlistmodel.h"

#include "qcm_interface/global.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/async.inl"
#include "qcm_interface/sql/meta_sql.h"

#include "Qcm/query/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/sql/collection_sql.h"
#include "Qcm/app.h"

namespace qcm::query
{
struct AlbumCollectionItem : public Album {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QDateTime, subTime, subTime)
};

class AlbumCollection : public meta_model::QGadgetListModel<AlbumCollectionItem> {
    Q_OBJECT
public:
    AlbumCollection(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<AlbumCollectionItem>(parent), m_has_more(true) {}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);

private:
    bool m_has_more;
};

class AlbumCollectionQuery : public Query<AlbumCollection> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumCollectionQuery(QObject* parent = nullptr): Query<AlbumCollection>(parent) {
        set_use_queue(true);
        connect(Notifier::instance(),
                &Notifier::collected,
                this,
                [this](const model::ItemId& id, bool) {
                    if (id.type() == u"album") {
                        request_reload();
                    }
                });
        connect(Notifier::instance(),
                &Notifier::collection_synced,
                this,
                [this](enums::CollectionType type) {
                    if (type == enums::CollectionType::CTAlbum) {
                        request_reload();
                    }
                });
    }

public:
    auto query_collect(const model::ItemId& userId, const QDateTime& time)
        -> task<std::vector<AlbumCollectionItem>> {
        auto                             sql = App::instance()->album_sql();
        std::vector<AlbumCollectionItem> items;
        co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0},
    collection.collectTime 
FROM album 
JOIN collection ON album.itemId = collection.itemId
LEFT JOIN album_artist ON album.itemId = album_artist.albumId
LEFT JOIN artist ON album_artist.artistId = artist.itemId
WHERE collection.userId = :userId AND collection.type = "album" AND collection.collectTime > :time AND collection.removed = 0
GROUP BY album.itemId
ORDER BY collection.collectTime DESC;
)",
                                     Album::sql().select));
        query.bindValue(":userId", userId.toUrl());
        query.bindValue(":time", time);

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        }
        while (query.next()) {
            auto& item = items.emplace_back();
            int   i    = 0;
            query::load_query<Album>(query, item, i);
            item.subTime = query.value(i++).toDateTime();
        }
        co_return items;
    }

    void reload() override {
        if (status() == Status::Uninitialized) {
            Action::instance()->sync_collection(enums::CollectionType::CTAlbum);
        }
        set_status(Status::Querying);
        auto userId = Global::instance()->qsession()->user()->userId();

        auto self = helper::QWatcher { this };

        auto time = last();
        spawn([self, userId, time] -> asio::awaitable<void> {
            auto sql     = App::instance()->collect_sql();
            auto missing = co_await sql->select_missing(
                userId, "album", "album", { "name", "picUrl", "trackCount" });

            auto deleted_vec = co_await sql->select_removed(userId, u"album"_s, time);
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
                            t->begin(), t->end(), el, [userId](const auto& el, const auto& val) {
                                return el.subTime > val.subTime;
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
};

} // namespace qcm::query
