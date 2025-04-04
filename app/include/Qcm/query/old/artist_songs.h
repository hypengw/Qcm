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

class ArtistSongs : public meta_model::QGadgetListModel<Song> {
    Q_OBJECT

public:
    ArtistSongs(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Song>(parent), m_has_more(true) {}

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

class ArtistSongsQuery : public QueryList<ArtistSongs> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(ArtistSongs* data READ tdata NOTIFY itemIdChanged FINAL)
public:
    ArtistSongsQuery(QObject* parent = nullptr): QueryList<ArtistSongs>(parent) {
        connect_requet_reload(&ArtistSongsQuery::itemIdChanged);
        connect(tdata(), &ArtistSongs::fetchMoreReq, this, &ArtistSongsQuery::setOffset);
    }

    auto itemId() const -> const model::ItemId& { return m_album_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_album_id, v)) {
            itemIdChanged();
        }
    }
    Q_SIGNAL void itemIdChanged();

public:
    auto query_artist_song_count(model::ItemId itemId) -> task<std::optional<i32>> {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare(u"SELECT musicCount FROM artist WHERE itemId = :itemId;"_s);
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            co_return query.value(0).toInt();
        }
        co_return std::nullopt;
    }

    auto query_songs(model::ItemId itemId, qint32 offset, qint32 limit)
        -> task<std::optional<std::vector<Song>>> {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0}
FROM song
JOIN album ON album.itemId = song.albumId
JOIN song_artist ON song.itemId = song_artist.songId
JOIN artist ON song_artist.artistId = artist.itemId
WHERE song_artist.artistId = :itemId
GROUP BY song.itemId
ORDER BY song.popularity DESC
LIMIT :limit OFFSET :offset;
)",
                                     Song::sql().select));

        query.bindValue(u":itemId"_s, itemId.toUrl());
        query.bindValue(u":offset"_s, offset);
        query.bindValue(u":limit"_s, limit);

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else {
            std::vector<Song> songs;
            while (query.next()) {
                auto& s = songs.emplace_back();
                int   i = 0;
                load_query(query, s, i);
            }
            co_return songs;
        }
        co_return std::nullopt;
    }

    void reload() override {
        set_status(Status::Querying);
        auto self   = helper::QWatcher { this };
        auto itemId = m_album_id;
        auto offset = this->offset();
        auto limit  = this->limit();

        spawn([self, itemId, offset, limit] -> task<void> {
            auto                     sql = App::instance()->item_sql();
            std::vector<model::Song> items;
            bool                     needReload = false;

            bool                             synced { false };
            std::optional<std::vector<Song>> songs;
            std::optional<error::Error>      error;
            for (;;) {
                auto count = co_await self->query_artist_song_count(itemId);
                songs      = co_await self->query_songs(itemId, offset, limit);
                if (! synced &&
                    (! count || ! songs ||
                     (offset + (int)songs->size() != count && (int)songs->size() < limit))) {
                    auto out = co_await SyncAPi::sync_list(
                        enums::SyncListType::CTArtistSong, itemId, offset, limit);
                    if (! out) {
                        error = out.error();
                        break;
                    }
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
            if (self) {
                if (error) {
                    self->set_error(convert_from<QString>(error->what()));
                    self->set_status(Status::Error);
                } else {
                    if (songs) {
                        self->tdata()->setHasMore(songs->size());
                        self->tdata()->insert(self->tdata()->rowCount(), songs.value());
                    }
                    self->set_status(Status::Finished);
                }
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}

private:
    model::ItemId m_album_id;
};

} // namespace qcm::query
