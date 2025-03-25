#include "Qcm/query/search.h"

#include "core/qasio/qt_sql.h"

#include "qcm_interface/async.inl"

#include "Qcm/sql/item_sql.h"
#include "Qcm/query/query_load.h"
#include "Qcm/app.h"

namespace qcm::query
{

SearchTypeModel::SearchTypeModel(QObject* parent)
    : meta_model::QGadgetListModel<SearchTypeItem>(parent), m_current_index(0) {
    connect(
        this, &SearchTypeModel::currentIndexChanged, this, &SearchTypeModel::currentTypeChanged);
    resetModel(std::array {
        SearchTypeItem { .name = "song", .type = enums::SearchType::SearchSong },
        SearchTypeItem { .name = "album", .type = enums::SearchType::SearchAlbum },
    });
}
auto SearchTypeModel::currentIndex() const -> qint32 { return m_current_index; }
void SearchTypeModel::setCurrentIndex(qint32 v) {
    if (ycore::cmp_exchange(m_current_index, v)) {
        currentIndexChanged();
    }
}

auto SearchTypeModel::currentType() const -> enums::SearchType {
    if (m_current_index < 0 || m_current_index >= rowCount()) {
        return enums::SearchType::SearchSong;
    }
    return at(m_current_index).type;
}
void SearchTypeModel::setCurrentType(enums::SearchType v) {
    for (qint32 i = 0; i < rowCount(); i++) {
        if (at(i).type == v) {
            setCurrentIndex(i);
            return;
        }
    }
};

SearchQuery::SearchQuery(QObject* parent)
    : QueryList<QAbstractListModel>({}, parent, nullptr),
      m_location(enums::SearchLocation::SearchLocal),
      m_type(enums::SearchType::SearchSong) {
    connect(this, &SearchQuery::typeChanged, this, [this](SearchType type) {
        if (type == enums::SearchType::SearchAlbum) {
            this->set_tdata(new SearchAlbumModel(this));
        } else {
            this->set_tdata(new SearchSongModel(this));
        }
    });
    typeChanged(m_type);
}

auto SearchQuery::text() const -> QString { return m_text; }
void SearchQuery::setText(QString v) {
    if (ycore::cmp_exchange(m_text, v)) {
        textChanged(m_text);
    }
}

auto SearchQuery::location() const -> enums::SearchLocation { return m_location; }
void SearchQuery::setLocation(enums::SearchLocation v) {
    if (ycore::cmp_exchange(m_location, v)) {
        locationChanged(m_location);
    }
};

auto SearchQuery::type() const -> enums::SearchType { return m_type; }
void SearchQuery::setType(enums::SearchType v) {
    if (ycore::cmp_exchange(m_type, v)) {
        typeChanged(m_type);
    };
}

auto SearchQuery::query_album(WatchSelf self, SearchLocation loc, QStringView text, i32 offset,
                              i32 limit) -> task<void> {
    std::vector<Album> albums;
    {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0}
FROM album 
JOIN album_fts ON album.rowid = album_fts.rowid
JOIN collection ON album.itemId = collection.itemId
LEFT JOIN album_artist ON album.itemId = album_artist.albumId
LEFT JOIN artist ON album_artist.artistId = artist.itemId
WHERE album_fts MATCH :text
GROUP BY album.itemId
LIMIT {2} OFFSET {1};
)",
                                     Album::sql().select,
                                     offset,
                                     limit));
        query.bindValue(":text", text.toString());
        if (query.exec()) {
            while (query.next()) {
                auto& album = albums.emplace_back();
                int   i     = 0;
                load_query(query, album, i);
            }
        }
    }

    co_await asio::post(asio::bind_executor(qcm::qexecutor(), use_task));
    if (! self) co_return;
    if (auto model = self->get_model<SearchAlbumModel>()) {
        model->resetModel(albums);
    }
    self->set_status(Status::Finished);
    co_return;
}
auto SearchQuery::query_song(WatchSelf self, SearchLocation loc, QStringView text, i32 offset,
                             i32 limit) -> task<void> {
    std::vector<Song> songs;
    {
        auto sql = App::instance()->item_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0}
FROM song
JOIN song_fts ON song.rowid = song_fts.rowid
JOIN collection ON song.itemId = collection.itemId OR song.albumId = collection.itemId
LEFT JOIN album ON song.albumId = album.itemId
LEFT JOIN song_artist ON song.itemId = song_artist.songId
LEFT JOIN artist ON song_artist.artistId = artist.itemId
WHERE song_fts MATCH :text
GROUP BY song.itemId
LIMIT {2} OFFSET {1};
)",
                                     Song::sql().select,
                                     offset,
                                     limit));
        query.bindValue(":text", text.toString());
        if (query.exec()) {
            while (query.next()) {
                auto& song = songs.emplace_back();
                int   i    = 0;
                load_query(query, song, i);
            }
        }
    }

    co_await asio::post(asio::bind_executor(qcm::qexecutor(), use_task));
    if (! self) co_return;
    if (auto model = self->get_model<SearchSongModel>()) {
        model->resetModel(songs);
    }
    self->set_status(Status::Finished);
    co_return;
}
void SearchQuery::reload() {
    set_status(Status::Querying);

    spawn([self   = WatchSelf(this),
           loc    = location(),
           text   = text(),
           type   = type(),
           offset = offset(),
           limit  = limit()]() -> task<void> {
        if (type == enums::SearchType::SearchAlbum) {
            co_await self->query_album(self, loc, text, offset, limit);
        } else {
            co_await self->query_song(self, loc, text, offset, limit);
        }
    });
}

} // namespace qcm::query