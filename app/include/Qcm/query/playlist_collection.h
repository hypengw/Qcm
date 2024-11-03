#pragma once

#include <QQmlEngine>

#include "Qcm/query/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
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
    PlaylistCollectionQuery(QObject* parent = nullptr): Query<PlaylistCollection>(parent) {}

public:
    void reload() override {
        if (status() == Status::Uninitialized) {
            Action::instance()->sync_collection(enums::CollectionType::CTPlaylist);
        }
        set_status(Status::Querying);
        auto userId = Global::instance()->qsession()->user()->userId();

        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        spawn(ex, [self, userId] -> asio::awaitable<void> {
            auto                                sql = App::instance()->album_sql();
            std::vector<PlaylistCollectionItem> items;
            {
                co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
                auto query = sql->con()->query();
                query.prepare(uR"(
SELECT 
    %1,
    collection.collectTime 
FROM playlist 
JOIN collection ON playlist.itemId = collection.itemId
WHERE collection.userId = :userId
GROUP BY playlist.itemId
ORDER BY collection.collectTime DESC;
)"_s.arg(model::Playlist::Select));
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