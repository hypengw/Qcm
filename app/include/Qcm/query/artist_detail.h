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
#include "qcm_interface/model/artist.h"
#include "qcm_interface/async.inl"

namespace qcm::query
{

class ArtistDetail : public QObject {
    Q_OBJECT

    Q_PROPERTY(Artist info READ info NOTIFY infoChanged)
public:
    ArtistDetail(QObject* parent = nullptr): QObject(parent) {}

    auto info() const -> const Artist& { return m_info; }
    void setInfo(const std::optional<Artist>& v) {
        m_info = v.value_or(Artist {});
        infoChanged();
    }

    Q_SIGNAL void infoChanged();

private:
    Artist m_info;
};

class ArtistDetailQuery : public Query<ArtistDetail> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
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
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0}
FROM artist 
WHERE itemId = :itemId AND ({1});
)",
                                     model::Artist::sql().select,
                                     db::null<db::AND, db::NOT>(model::Artist::sql().columns)));
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            Artist artist;
            int    i = 0;
            query::load_query(query, artist, i);
            co_return artist;
        }
        co_return std::nullopt;
    }

    void reload() override {
        set_status(Status::Querying);
        auto self   = helper::QWatcher { this };
        auto itemId = m_album_id;
        spawn( [self, itemId] -> task<void> {
            auto                     sql = App::instance()->item_sql();
            std::vector<model::Song> items;

            bool                                     synced { 0 };
            std::optional<Artist>                    artist;
            std::optional<std::vector<model::Album>> albums;
            for (;;) {
                artist = co_await self->query_artist(itemId);
                if (! synced && ! artist) {
                    co_await SyncAPi::sync_item(itemId);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
            if (self) {
                self->tdata()->setInfo(artist);
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
