#pragma once

#pragma once

#include <QQmlEngine>

#include "qcm_interface/query.h"
#include "Qcm/query/query_load.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.hpp"
#include "core/qasio/qt_sql.h"
#include "Qcm/global.hpp"
#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/macro.hpp"
#include "Qcm/model/album.hpp"
#include "Qcm/model/artist.hpp"
#include "Qcm/util/async.inl"

namespace qcm::query
{

class ArtistAlbums : public meta_model::QGadgetListModel<model::Album> {
    Q_OBJECT

public:
    ArtistAlbums(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<model::Album>(parent), m_has_more(true) {}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    void setHasMore(bool v) { m_has_more = v; }

    Q_SIGNAL void fetchMoreReq(qint32);

private:
    bool m_has_more;
};

class ArtistAlbumsQuery : public QueryList<ArtistAlbums> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(ArtistAlbums* data READ tdata NOTIFY itemIdChanged FINAL)
public:
    ArtistAlbumsQuery(QObject* parent = nullptr): QueryList<ArtistAlbums>(parent) {
        connect_requet_reload(&ArtistAlbumsQuery::itemIdChanged);
        connect(tdata(), &ArtistAlbums::fetchMoreReq, this, &ArtistAlbumsQuery::setOffset);
    }

    auto itemId() const -> const model::ItemId& { return m_album_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_album_id, v)) {
            itemIdChanged();
        }
    }

    Q_SIGNAL void itemIdChanged();

public:
    auto query_artist_album_count(model::ItemId itemId) -> task<std::optional<i32>> {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare(u"SELECT albumCount FROM artist WHERE itemId = :itemId;"_s);
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            co_return query.value(0).toInt();
        }
        co_return std::nullopt;
    }
    auto query_albums(model::ItemId itemId, qint32 offset, qint32 limit)
        -> task<std::optional<std::vector<model::Album>>> {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare_sv(std::format(R"(
SELECT 
    {0}
FROM album
JOIN album_artist ON album.itemId = album_artist.albumId
JOIN artist ON album_artist.artistId = artist.itemId
WHERE album_artist.artistId = :itemId
GROUP BY album.itemId
ORDER BY album.publishTime DESC
LIMIT :limit OFFSET :offset;
)",
                                     Album::sql().select));

        query.bindValue(":itemId", itemId.toUrl());
        query.bindValue(":offset", offset);
        query.bindValue(":limit", limit);

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else {
            std::vector<model::Album> albums;
            while (query.next()) {
                auto& al = albums.emplace_back();
                int   i  = 0;
                load_query( query, al, i);
            }
            co_return albums;
        }
        co_return std::nullopt;
    }

    void reload() override {
        set_status(Status::Querying);
        auto self   = helper::QWatcher { this };
        auto itemId = m_album_id;
        auto offset = self->offset();
        auto limit  = self->limit();
        spawn( [self, itemId, offset, limit] -> task<void> {
            auto                     sql = App::instance()->item_sql();
            std::vector<model::Song> items;
            bool                     needReload = false;

            bool                                     synced { 0 };
            std::optional<std::vector<model::Album>> albums;
            for (;;) {
                auto count = co_await self->query_artist_album_count(itemId);
                albums     = co_await self->query_albums(itemId, offset, limit);
                if (! synced &&
                    (! count || ! albums ||
                     (offset + (int)albums->size() != count && (int)albums->size() < limit))) {
                    co_await SyncAPi::sync_list(
                        enums::SyncListType::CTArtistAlbum, itemId, offset, limit);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
            if (self) {
                if (albums) {
                    self->tdata()->setHasMore(albums->size());
                    self->tdata()->insert(self->tdata()->rowCount(), albums.value());
                }
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}

private:
    model::ItemId m_album_id;
};

} // namespace qcm::query
