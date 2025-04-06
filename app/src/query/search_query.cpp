#include "Qcm/query/search_query.hpp"

#include "Qcm/util/async.inl"
#include "Qcm/app.hpp"

namespace qcm
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
    : query::QueryList<QAbstractListModel>({}, parent, nullptr),
      m_location(enums::SearchLocation::SearchLocal),
      m_type(enums::SearchType::SearchSong) {
    connect(this, &SearchQuery::typeChanged, this, [this](SearchType type) {
        if (type == enums::SearchType::SearchAlbum) {
            this->set_tdata(new model::SearchAlbumModel(this));
        } else {
            this->set_tdata(new model::SearchSongModel(this));
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

void SearchQuery::reload() {
    // set_status(Status::Querying);
    // spawn([self   = WatchSelf(this),
    //        loc    = location(),
    //        text   = text(),
    //        type   = type(),
    //        offset = offset(),
    //        limit  = limit()]() -> task<void> {
    //     if (type == enums::SearchType::SearchAlbum) {
    //         co_await self->query_album(self, loc, text, offset, limit);
    //     } else {
    //         co_await self->query_song(self, loc, text, offset, limit);
    //     }
    // });
}

} // namespace qcm

#include "Qcm/query/moc_search_query.cpp"