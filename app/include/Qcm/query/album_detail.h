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

class AlbumDetail : public meta_model::QGadgetListModel<Song> {
    Q_OBJECT

    Q_PROPERTY(Album info READ info NOTIFY infoChanged)
public:
    AlbumDetail(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Song>(parent), m_has_more(true) {}

    auto info() const -> const Album& { return m_info; }
    void setInfo(const std::optional<Album>& v) {
        m_info = v.value_or(Album {});
        infoChanged();
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);
    Q_SIGNAL void infoChanged();

private:
    bool  m_has_more;
    Album m_info;
};

class AlbumDetailQuery : public Query<AlbumDetail> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(AlbumDetail* data READ tdata NOTIFY itemIdChanged FINAL)
public:
    AlbumDetailQuery(QObject* parent = nullptr): Query<AlbumDetail>(parent) {}

    auto itemId() const -> const model::ItemId& { return m_album_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_album_id, v)) {
            itemIdChanged();
        }
    }

    Q_SIGNAL void itemIdChanged();

public:
    auto query_album(model::ItemId itemId) -> task<std::optional<Album>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare(uR"(
SELECT 
    album.itemId, 
    album.name, 
    album.picUrl, 
    album.trackCount,
    album.publishTime,
    GROUP_CONCAT(artist.itemId) AS artistIds, 
    GROUP_CONCAT(artist.name) AS artistNames,
    GROUP_CONCAT(artist.picUrl) AS artistPicUrls
FROM album
JOIN album_artist ON album.itemId = album_artist.albumId
JOIN artist ON album_artist.artistId = artist.itemId
WHERE album.itemId = :itemId
GROUP BY album.itemId;
)"_s);
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            Album album;
            int   i           = 0;
            album.id          = query.value(i++).toUrl();
            album.name        = query.value(i++).toString();
            album.picUrl      = query.value(i++).toString();
            album.trackCount  = query.value(i++).toInt();
            album.publishTime = query.value(i++).toDateTime();
            {
                auto artist_ids     = query.value(i++).toStringList();
                auto artist_names   = query.value(i++).toStringList();
                auto artist_picUrls = query.value(i++).toStringList();
                for (qsizetype i = 0; i < artist_ids.size(); i++) {
                    auto& ar  = album.artists.emplace_back();
                    ar.id     = artist_ids[i];
                    ar.name   = artist_names[i];
                    ar.picUrl = artist_picUrls[i];
                }
            }
            co_return album;
        }
        co_return std::nullopt;
    }

    auto query_songs(model::ItemId itemId) -> task<std::optional<std::vector<Song>>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare(uR"(
SELECT 
    song.itemId, 
    song.name, 
    COALESCE(song.coverUrl, album.picUrl) AS picUrl,
    song.canPlay,
    album.itemId,
    album.name,
    GROUP_CONCAT(artist.itemId) AS artistIds, 
    GROUP_CONCAT(artist.name) AS artistNames,
    GROUP_CONCAT(artist.picUrl) AS artistPicUrls
FROM song
JOIN album ON song.albumId = album.itemId
JOIN song_artist ON song.itemId = song_artist.songId
JOIN artist ON song_artist.artistId = artist.itemId
WHERE song.albumId = :itemId
GROUP BY song.itemId
ORDER BY song.trackNumber ASC;
)"_s);

        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else {
            std::vector<Song> songs;
            while (query.next()) {
                auto& s     = songs.emplace_back();
                int   i     = 0;
                s.id        = query.value(i++).toUrl();
                s.name      = query.value(i++).toString();
                s.coverUrl  = query.value(i++).toString();
                s.canPlay   = query.value(i++).toInt();
                s.albumId   = query.value(i++).toUrl();
                s.albumName = query.value(i++).toString();
                {
                    auto artist_ids     = query.value(i++).toStringList();
                    auto artist_names   = query.value(i++).toStringList();
                    auto artist_picUrls = query.value(i++).toStringList();
                    for (qsizetype i = 0; i < artist_ids.size(); i++) {
                        auto& ar  = s.artists.emplace_back();
                        ar.id     = artist_ids[i];
                        ar.name   = artist_names[i];
                        ar.picUrl = artist_picUrls[i];
                    }
                }
            }
            co_return songs;
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

            bool                             synced { 0 };
            std::optional<Album>             album;
            std::optional<std::vector<Song>> songs;
            for (;;) {
                album = co_await self->query_album(itemId);
                songs = co_await self->query_songs(itemId);
                if (! synced && (! album || ! songs || album->trackCount != (int)songs->size())) {
                    co_await SyncAPi::sync_item(itemId);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
            if (self) {
                self->tdata()->setInfo(album);
                self->tdata()->resetModel(songs);
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
