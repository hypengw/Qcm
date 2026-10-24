module;
#include "Qcm/query/search_query.moc.h"
module qcm;
import :query.search;
import cppstd;

namespace qcm
{

SearchTypeModel::SearchTypeModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent), m_current_index(0) {
    resetModel(std::array {
        SearchTypeItem { .name = "song", .type = enums::SearchType::SearchSong },
        SearchTypeItem { .name = "album", .type = enums::SearchType::SearchAlbum },
        SearchTypeItem { .name = "artist", .type = enums::SearchType::SearchArtist },
    });
}
auto SearchTypeModel::currentIndex() const -> qint32 { return m_current_index; }
void SearchTypeModel::setCurrentIndex(qint32 v) {
    if (ycore::cmp_set(m_current_index, v)) {
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

namespace model
{

SearchModel::SearchModel(QObject* parent)
    : QObject(parent),
      m_album_model(new AlbumListModel(this)),
      m_song_model(new SongListModel(this)),
      m_artist_model(new ArtistListModel(this)) {
}

auto SearchModel::songs() -> SongListModel* { return m_song_model; }
auto SearchModel::albums() -> AlbumListModel* { return m_album_model; }
auto SearchModel::artists() -> ArtistListModel* { return m_artist_model; }

auto SearchModel::songQuery() const -> QString { return m_song_query; }
auto SearchModel::albumQuery() const -> QString { return m_album_query; }
auto SearchModel::artistQuery() const -> QString { return m_artist_query; }

void SearchModel::setSongQuery(const QString& v) {
    if (ycore::cmp_set(m_song_query, v)) {
        songQueryChanged(m_song_query);
    }
}
void SearchModel::setAlbumQuery(const QString& v) {
    if (ycore::cmp_set(m_album_query, v)) {
        albumQueryChanged(m_album_query);
    }
}
void SearchModel::setArtistQuery(const QString& v) {
    if (ycore::cmp_set(m_artist_query, v)) {
        artistQueryChanged(m_artist_query);
    }
}

} // namespace model

SearchQuery::SearchQuery(QObject* parent)
    : QueryList(parent),
      QueryExtra<model::SearchModel, SearchQuery>(true),
      m_location(enums::SearchLocation::SearchLocal),
      m_type(enums::SearchType::SearchSong) {}

auto SearchQuery::text() const -> QString { return m_text; }
void SearchQuery::setText(QString v) {
    if (ycore::cmp_set(m_text, v)) {
        invalidateRequests();
        textChanged(m_text);
    }
}

auto SearchQuery::location() const -> enums::SearchLocation { return m_location; }
void SearchQuery::setLocation(enums::SearchLocation v) {
    if (ycore::cmp_set(m_location, v)) {
        invalidateRequests();
        locationChanged(m_location);
    }
};
auto SearchQuery::data() const -> model::SearchModel* { return this->tdata(); }
void SearchQuery::setData(model::SearchModel* v) { return this->set_tdata(v); }

auto SearchQuery::type() const -> enums::SearchType { return m_type; }
void SearchQuery::setType(enums::SearchType v) {
    if (ycore::cmp_set(m_type, v)) {
        invalidateRequests();
        typeChanged(m_type);
    };
}

void SearchQuery::reload() {
    setStatus(Status::Querying);
    auto app = App::instance();
    auto req = msg::SearchReq {};
    req.setQuery(text());
    req.setPage(offset());
    req.setPageSize(limit());
    req.setLibraryId(app->libraryStatus()->activedIds());

    {
        QList<QtProtobuf::int32> types;
        types.push_back((qint32)type());
        req.setTypes(types);
    }

    const auto revision = beginRequest();
    spawn([self = WatchSelf(this), req, revision]() mutable -> task<void> {
        auto backend = App::instance()->backend();
        auto query   = req.query();
        auto rsp     = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || ! self->isCurrentRequest(revision)) co_return;
        self->inspect_set(rsp, [self, query](msg::SearchRsp& rsp) {
            if (rsp.hasAlbums()) {
                auto el = rsp.albums();
                auto t  = self->tdata()->albums();
                t->setHasMore(false);
                t->syncIds(el.ids());

                t->setHasMore(el.hasMore());
                self->tdata()->setAlbumQuery(query);
            }

            if (rsp.hasSongs()) {
                auto el = rsp.songs();
                auto t  = self->tdata()->songs();
                t->setHasMore(false);
                t->syncIds(el.ids());

                t->setHasMore(el.hasMore());
                self->tdata()->setSongQuery(query);
            }

            if (rsp.hasArtists()) {
                auto el = rsp.artists();
                auto t  = self->tdata()->artists();
                t->setHasMore(false);
                t->syncIds(el.ids());
                t->setHasMore(el.hasMore());
                self->tdata()->setArtistQuery(query);
            }
        });

        co_return;
    });
}

} // namespace qcm

#include "Qcm/query/search_query.moc.cpp"
