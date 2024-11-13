#pragma once

#include <QQmlEngine>

#include "asio_qt/qt_sql.h"
#include "meta_model/qgadgetlistmodel.h"

#include "qcm_interface/global.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/async.inl"

#include "qcm_interface/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.h"

namespace qcm::query
{

class SongDetailQuery : public Query<Song> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(qcm::query::Song data READ tdata NOTIFY dataChanged FINAL)
public:
    SongDetailQuery(QObject* parent = nullptr): Query<Song>(parent) {
        connect(this, &SongDetailQuery::itemIdChanged, this, &SongDetailQuery::reload);
    }

    auto itemId() const -> const model::ItemId& { return m_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_id, v)) {
            itemIdChanged();
        }
    }

    Q_SIGNAL void itemIdChanged();

public:
    auto query_song(model::ItemId itemId) -> task<std::optional<Song>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0}
FROM song
JOIN album ON song.albumId = album.itemId
JOIN song_artist ON song.itemId = song_artist.songId
JOIN artist ON song_artist.artistId = artist.itemId
WHERE song.itemId = :itemId AND ({1})
GROUP BY song.itemId;
)",
                                     Song::sql().select,
                                     db::null<db::AND, db::NOT>(Song::sql().columns)));

        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{},\n{}", query.lastQuery(), query.lastError().text());
        } else {
            if (query.next()) {
                Song song;
                int  i = 0;
                load_query(query, song, i);
                co_return song;
            }
        }
        co_return std::nullopt;
    }

    void reload() override {
        auto itemId = m_id;
        if (! itemId.valid()) return;

        auto self = helper::QWatcher { this };
        set_status(Status::Querying);

        spawn([self, itemId] -> task<void> {
            auto                     sql = App::instance()->album_sql();
            std::vector<model::Song> items;

            bool                synced { 0 };
            std::optional<Song> song;
            for (;;) {
                song = co_await self->query_song(itemId);
                if (! synced && ! song) {
                    co_await SyncAPi::sync_item(itemId);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
            if (self) {
                self->set_tdata(song.value_or(Song {}));
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}

private:
    model::ItemId m_id;
};

} // namespace qcm::query
