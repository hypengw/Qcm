#pragma once

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

class ArtistDetail : public meta_model::QGadgetListModel<model::Album> {
    Q_OBJECT

    Q_PROPERTY(Artist info READ info NOTIFY infoChanged)
    Q_PROPERTY(std::vector<Song> hotSong READ hotSong NOTIFY hotSongChanged)
public:
    ArtistDetail(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<model::Album>(parent), m_has_more(true) {}

    auto info() const -> const Artist& { return m_info; }
    void setInfo(const std::optional<Artist>& v) {
        m_info = v.value_or(Artist {});
        infoChanged();
    }

    auto hotSong() const -> const std::vector<Song>& { return m_hot_song; }
    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);
    Q_SIGNAL void infoChanged();
    Q_SIGNAL void hotSongChanged();

private:
    bool              m_has_more;
    Artist            m_info;
    std::vector<Song> m_hot_song;
};

class ArtistDetailQuery : public Query<ArtistDetail> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(ArtistDetail* data READ tdata NOTIFY itemIdChanged FINAL)
public:
    ArtistDetailQuery(QObject* parent = nullptr): Query<ArtistDetail>(parent) {
        connect(this, &ArtistDetailQuery::itemIdChanged, this, &ArtistDetailQuery::reload);
    }

    auto itemId() const -> const model::ItemId& { return m_album_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_album_id, v)) {
            itemIdChanged();
        }
    }

    Q_SIGNAL void itemIdChanged();

public:
    auto query_artist(model::ItemId itemId) -> task<std::optional<Artist>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare(uR"(
SELECT 
    itemId, 
    name, 
    picUrl, 
    albumCount,
    musicCount
FROM artist 
WHERE itemId = :itemId;
)"_s);
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            Artist artist;
            int    i          = 0;
            artist.id         = query.value(i++).toUrl();
            artist.name       = query.value(i++).toString();
            artist.picUrl     = query.value(i++).toString();
            artist.albumCount = query.value(i++).toInt();
            artist.musicCount = query.value(i++).toInt();
            co_return artist;
        }
        co_return std::nullopt;
    }

    auto query_albums(model::ItemId itemId) -> task<std::optional<std::vector<model::Album>>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare(uR"(
SELECT 
    %1
FROM album
JOIN album_artist ON album.itemId = album_artist.albumId
JOIN artist ON album_artist.artistId = artist.itemId
WHERE album_artist.artistId = :itemId
GROUP BY album.itemId
ORDER BY album.publishTime DESC;
)"_s.arg(Album::Select));

        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else {
            std::vector<model::Album> albums;
            while (query.next()) {
                auto& al = albums.emplace_back();
                int   i  = 0;
                load_query(al, query, i);
            }
            co_return albums;
        }
        co_return std::nullopt;
    }

    void reload() override {
        set_status(Status::Querying);
        auto ex     = asio::make_strand(pool_executor());
        auto self   = helper::QWatcher { this };
        auto itemId = m_album_id;
        spawn(ex, [self, itemId] -> task<void> {
            auto                     sql = App::instance()->album_sql();
            std::vector<model::Song> items;
            bool                     needReload = false;

            bool                                     synced { 0 };
            std::optional<Artist>                    artist;
            std::optional<std::vector<model::Album>> albums;
            for (;;) {
                artist = co_await self->query_artist(itemId);
                albums = co_await self->query_albums(itemId);
                if (! synced &&
                    (! artist || ! albums || artist->albumCount != (int)albums->size())) {
                    co_await SyncAPi::sync_item(itemId);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
            if (self) {
                self->tdata()->setInfo(artist);
                self->tdata()->resetModel(albums);
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
