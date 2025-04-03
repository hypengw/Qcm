#pragma once

#pragma once

#include <QQmlEngine>

#include "qcm_interface/query.h"
#include "Qcm/query/query_load.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.h"
#include "core/qasio/qt_sql.h"
#include "qcm_interface/global.h"
#include "meta_model/qgadget_list_model.hpp"
#include "qcm_interface/macro.h"
#include "Qcm/model/album.hpp"
#include "Qcm/model/artist.hpp"
#include "qcm_interface/async.inl"

namespace qcm::query
{

class MixDetail : public meta_model::QGadgetListModel<Song> {
    Q_OBJECT

    Q_PROPERTY(qcm::model::Mix info READ info NOTIFY infoChanged)
public:
    MixDetail(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Song>(parent), m_has_more(true) {}

    auto info() const -> const model::Mix& { return m_info; }
    void setInfo(const std::optional<model::Mix>& v) {
        m_info = v.value_or(model::Mix {});
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
    bool       m_has_more;
    model::Mix m_info;
};

class MixDetailQuery : public Query<MixDetail> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(bool querySong READ querySong WRITE setQuerySong NOTIFY querySongChanged)
    Q_PROPERTY(MixDetail* data READ tdata NOTIFY itemIdChanged FINAL)
public:
    MixDetailQuery(QObject* parent = nullptr): Query<MixDetail>(parent), m_query_song(false) {
        connect(this, &MixDetailQuery::itemIdChanged, this, &MixDetailQuery::reload);
        connect(
            Notifier::instance(), &Notifier::itemChanged, this, [this](const model::ItemId& id) {
                if (this->itemId() == id) {
                    request_reload();
                }
            });
    }

    auto itemId() const -> const model::ItemId& { return m_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_id, v)) {
            itemIdChanged();
        }
    }

    auto querySong() const -> bool { return m_query_song; };
    void setQuerySong(bool v) {
        if (ycore::cmp_exchange(m_query_song, v)) {
            querySongChanged();
        }
    }

    Q_SIGNAL void itemIdChanged();
    Q_SIGNAL void querySongChanged();

public:
    auto query_mix(const model::ItemId& itemId, QDateTime& editTime)
        -> task<std::optional<model::Mix>> {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0},
    _editTime
FROM playlist
WHERE playlist.itemId = :itemId AND ({1});
)",
                                     model::Mix::sql().select,
                                     db::null<db::AND, db::NOT>(model::Mix::sql().columns)));
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            model::Mix pl;
            int        i = 0;
            query::load_query(query, pl, i);
            editTime = query.value(i++).toDateTime();
            co_return pl;
        }
        co_return std::nullopt;
    }

    auto query_songs(model::ItemId itemId) -> task<std::optional<std::vector<Song>>> {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0}
FROM song
JOIN album ON song.albumId = album.itemId
JOIN song_artist ON song.itemId = song_artist.songId
JOIN artist ON song_artist.artistId = artist.itemId
JOIN playlist_song ON playlist_song.songId = song.itemId
WHERE playlist_song.playlistId = :itemId
GROUP BY song.itemId
ORDER BY playlist_song.orderIdx;
)",
                                     Song::sql().select));

        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else {
            std::vector<Song> songs;
            while (query.next()) {
                auto& s = songs.emplace_back();
                int   i = 0;
                load_query(query, s, i);
                s.sourceId = itemId;
            }
            co_return songs;
        }
        co_return std::nullopt;
    }

    void reload() override {
        set_status(Status::Querying);
        auto self      = helper::QWatcher { this };
        auto itemId    = m_id;
        auto querySong = m_query_song;
        spawn([self, itemId, querySong] -> task<void> {
            auto sql = App::instance()->item_sql();

            bool                             synced { 0 };
            std::optional<model::Mix>        mix;
            std::optional<std::vector<Song>> songs;
            QDateTime                        editTime;
            for (;;) {
                mix = co_await self->query_mix(itemId, editTime);
                if (querySong) songs = co_await self->query_songs(itemId);
                bool songOld = querySong && (! songs || mix->trackCount != (int)songs->size());
                // half hour
                auto duration = editTime.secsTo(QDateTime::currentDateTime());
                bool timeOld  = duration > 60 * 60 * 2;
                if (! synced && (! mix || songOld || timeOld)) {
                    co_await SyncAPi::sync_item(itemId);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
            if (self) {
                self->tdata()->setInfo(mix);
                if (querySong) {
                    if (self->tdata()->rowCount() && songs) {
                        self->tdata()->replaceResetModel(*songs);
                    } else {
                        self->tdata()->resetModel(songs);
                    }
                }
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}

private:
    bool          m_query_song;
    model::ItemId m_id;
};

} // namespace qcm::query
