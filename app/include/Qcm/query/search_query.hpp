#pragma once

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/backend_msg.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/model/list_models.hpp"

namespace qcm
{

struct SearchTypeItem {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(qcm::enums::SearchType, type, type)
};

class SearchTypeModel : public meta_model::QGadgetListModel<SearchTypeItem> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged FINAL)
    Q_PROPERTY(qcm::enums::SearchType currentType READ currentType WRITE setCurrentType NOTIFY
                   currentIndexChanged FINAL)
public:
    SearchTypeModel(QObject* parent = nullptr);
    auto currentIndex() const -> qint32;
    void setCurrentIndex(qint32 v);

    auto currentType() const -> enums::SearchType;
    void setCurrentType(enums::SearchType v);

    Q_SIGNAL void currentIndexChanged();

private:
    qint32 m_current_index;
};

namespace model
{

class SearchSongModel : public meta_model::QGadgetListModel<Song> {
public:
    using base_type = meta_model::QGadgetListModel<Song>;
    using base_type::base_type;
};

class SearchAlbumModel : public meta_model::QGadgetListModel<Album> {
public:
    using base_type = meta_model::QGadgetListModel<Album>;
    using base_type::base_type;
};

class SearchModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::SongListModel* songs READ songs CONSTANT FINAL)
    Q_PROPERTY(qcm::model::AlbumListModel* albums READ albums CONSTANT FINAL)
    Q_PROPERTY(qcm::model::ArtistListModel* artists READ artists CONSTANT FINAL)

    Q_PROPERTY(QString songQuery READ songQuery NOTIFY songQueryChanged FINAL)
    Q_PROPERTY(QString albumQuery READ albumQuery NOTIFY albumQueryChanged FINAL)
    Q_PROPERTY(QString artistQuery READ artistQuery NOTIFY artistQueryChanged FINAL)
public:
    SearchModel(QObject* parent = nullptr);

    auto songs() -> model::SongListModel*;
    auto albums() -> model::AlbumListModel*;
    auto artists() -> model::ArtistListModel*;

    auto songQuery() const -> QString;
    auto albumQuery() const -> QString;
    auto artistQuery() const -> QString;

    void setSongQuery(const QString& v);
    void setAlbumQuery(const QString& v);
    void setArtistQuery(const QString& v);

    Q_SIGNAL void songQueryChanged(const QString&);
    Q_SIGNAL void albumQueryChanged(const QString&);
    Q_SIGNAL void artistQueryChanged(const QString&);

private:
    QString m_song_query;
    QString m_album_query;
    QString m_artist_query;

    model::AlbumListModel*  m_album_model;
    model::SongListModel*   m_song_model;
    model::ArtistListModel* m_artist_model;
};

} // namespace model

class SearchQuery : public QueryList, public QueryExtra<model::SearchModel, SearchQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::SearchModel* data READ data WRITE setData NOTIFY dataChanged FINAL)
    Q_PROPERTY(qcm::enums::SearchLocation location READ location WRITE setLocation NOTIFY
                   locationChanged FINAL)
    Q_PROPERTY(qcm::enums::SearchType type READ type WRITE setType NOTIFY typeChanged FINAL)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
public:
    using Self           = SearchQuery;
    using WatchSelf      = helper::QWatcher<SearchQuery>;
    using SearchType     = enums::SearchType;
    using SearchLocation = enums::SearchLocation;

    SearchQuery(QObject* parent = nullptr);

    auto          text() const -> QString;
    void          setText(QString v);
    Q_SIGNAL void textChanged(const QString&);
    auto          data() const -> model::SearchModel*;
    void          setData(model::SearchModel* v);

    auto          location() const -> SearchLocation;
    void          setLocation(SearchLocation v);
    Q_SIGNAL void locationChanged(SearchLocation);

    auto type() const -> SearchType;
    void setType(SearchType v);

    Q_SIGNAL void typeChanged(SearchType);

    void reload() override;

private:
    SearchLocation m_location;
    SearchType     m_type;
    QString        m_text;
};

} // namespace qcm