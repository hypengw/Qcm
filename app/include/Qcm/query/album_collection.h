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

    // void handle_output(const out_type& re, const auto& input) {
    //     if (input.offset == 0) {
    //         auto in_ = convert_from<std::vector<AlbumSublistItem>>(re.data);
    //         // convertModel(in_, [](const AlbumSublistItem& it) -> std::string {
    //         //     return convert_from<std::string>(it.id);
    //         // });
    //     } else if (input.offset == (int)rowCount()) {
    //         insert(rowCount(), convert_from<std::vector<AlbumSublistItem>>(re.data));
    //     }
    //     m_has_more = re.hasMore;
    // }

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
    AlbumCollectionQuery(QObject* parent = nullptr): Query<AlbumCollection>(parent) {}

    // FORWARD_PROPERTY(qint32, offset, offset)
    // FORWARD_PROPERTY(qint32, limit, limit)

public:
    // void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
    void reload() override {
        if (status() == Status::Uninitialized) {
            Action::instance()->sync_collection(enums::CollectionType::CTAlbum);
        }
        set_status(Status::Querying);
        auto userId = Global::instance()->qsession()->user()->userId();

        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        spawn(ex, [self, userId] -> asio::awaitable<void> {
            auto                             sql = App::instance()->album_sql();
            std::vector<AlbumCollectionItem> items;
            co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
            auto query = sql->con()->query();
            query.prepare(uR"(
SELECT 
    album.itemId, 
    album.name, 
    album.picUrl, 
    album.trackCount, 
    collection.collectTime, 
    GROUP_CONCAT(artist.itemId) AS artistIds, 
    GROUP_CONCAT(artist.name) AS artistNames,
    GROUP_CONCAT(artist.picUrl) AS artistPicUrls
FROM album
JOIN collection ON album.itemId = collection.itemId
JOIN album_artist ON album.itemId = album_artist.albumId
JOIN artist ON album_artist.artistId = artist.itemId
WHERE collection.userId = :userId
GROUP BY album.itemId
ORDER BY collection.collectTime DESC;
)"_s);
            query.bindValue(":userId", userId.toUrl());

            if (! query.exec()) {
                ERROR_LOG("{}", query.lastError().text());
            }
            while (query.next()) {
                auto& item      = items.emplace_back();
                item.id         = query.value(0).toUrl();
                item.name       = query.value(1).toString();
                item.picUrl     = query.value(2).toString();
                item.trackCount = query.value(3).toInt();
                item.subTime    = query.value(4).toDateTime();

                {
                    auto artist_ids     = query.value(5).toStringList();
                    auto artist_names   = query.value(6).toStringList();
                    auto artist_picUrls = query.value(7).toStringList();
                    for (qsizetype i = 0; i < artist_ids.size(); i++) {
                        auto& ar  = item.artists.emplace_back();
                        ar.id     = artist_ids[i];
                        ar.name   = artist_names[i];
                        ar.picUrl = artist_picUrls[i];
                    }
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

    Q_SLOT void reset() {
        // api().input.offset = 0;
        // reload();
    }
};

} // namespace qcm::query
