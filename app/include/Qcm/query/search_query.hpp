#pragma once

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/query/query.hpp"
#include "Qcm/backend_msg.hpp"

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
                   currentTypeChanged FINAL)
public:
    SearchTypeModel(QObject* parent = nullptr);
    auto currentIndex() const -> qint32;
    void setCurrentIndex(qint32 v);

    auto currentType() const -> enums::SearchType;
    void setCurrentType(enums::SearchType v);

    Q_SIGNAL void currentIndexChanged();
    Q_SIGNAL void currentTypeChanged();

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

} // namespace model

class SearchQuery : public QueryList<QAbstractListModel> {
    Q_OBJECT
    QML_ELEMENT

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

    auto          location() const -> SearchLocation;
    void          setLocation(SearchLocation v);
    Q_SIGNAL void locationChanged(SearchLocation);

    auto type() const -> SearchType;
    void setType(SearchType v);

    Q_SIGNAL void typeChanged(SearchType);

    template<typename T>
    T* get_model() {
        return data().value<T*>();
    }

    void reload() override;

private:
    SearchLocation m_location;
    SearchType     m_type;
    QString        m_text;
};

} // namespace qcm