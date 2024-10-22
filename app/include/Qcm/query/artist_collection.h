#pragma once

#include <QQmlEngine>

#include "Qcm/query/query.h"
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
struct ArtistCollectionItem : public model::Artist {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QDateTime, subTime, subTime)
};

class ArtistCollection : public meta_model::QGadgetListModel<ArtistCollectionItem> {
    Q_OBJECT
public:
    ArtistCollection(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<ArtistCollectionItem>(parent), m_has_more(true) {}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);

private:
    bool m_has_more;
};

class ArtistCollectionQuery : public Query<ArtistCollection> {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistCollectionQuery(QObject* parent = nullptr): Query<ArtistCollection>(parent) {}

public:
    void reload() override {
        if (status() == Status::Uninitialized) {
            Action::instance()->sync_collection(enums::CollectionType::CTArtist);
        }
        set_status(Status::Querying);
        auto userId = Global::instance()->qsession()->user()->userId();

        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        spawn(ex, [self, userId] -> asio::awaitable<void> {
            auto                              sql = App::instance()->album_sql();
            std::vector<ArtistCollectionItem> items;
            co_await asio::post(asio::bind_executor(sql->get_executor(), asio::use_awaitable));
            auto query = sql->con()->query();
            query.prepare(uR"(
SELECT 
    artist.itemId, 
    artist.name, 
    artist.picUrl, 
    artist.albumCount, 
    collection.collectTime
FROM artist
JOIN collection ON artist.itemId = collection.itemId
WHERE collection.userId = :userId
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
                item.albumCount = query.value(3).toInt();
                item.subTime    = query.value(4).toDateTime();
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
