#pragma once

#include "qcm_interface/query.h"
#include "meta_model/qgadgetlistmodel.h"

namespace qcm::query
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
    SearchTypeModel(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<SearchTypeItem>(parent), m_current_index(0) {
        connect(this,
                &SearchTypeModel::currentIndexChanged,
                this,
                &SearchTypeModel::currentTypeChanged);
        resetModel(std::array {
            SearchTypeItem { .name = "song", .type = enums::SearchType::SearchSong },
            SearchTypeItem { .name = "album", .type = enums::SearchType::SearchAlbum },
        });
    }
    auto currentIndex() -> qint32 { return m_current_index; }
    void setCurrentIndex(qint32 v) {
        if (ycore::cmp_exchange(m_current_index, v)) {
            currentIndexChanged();
        }
    }

    auto currentType() -> enums::SearchType {
        if (m_current_index < 0 || m_current_index >= rowCount()) {
            return enums::SearchType::SearchSong;
        }
        return at(m_current_index).type;
    }
    void setCurrentType(enums::SearchType v) {
        for (qint32 i = 0; i < rowCount(); i++) {
            if (at(i).type == v) {
                setCurrentIndex(i);
                return;
            }
        }
    };

    Q_SIGNAL void currentIndexChanged();
    Q_SIGNAL void currentTypeChanged();

private:
    qint32 m_current_index;
};

class SearchQuery : public QueryBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::enums::SearchLocation location READ location WRITE setLocation NOTIFY
                   locationChanged FINAL)
    Q_PROPERTY(qcm::enums::SearchType type READ type WRITE setType NOTIFY typeChanged FINAL)
public:
    SearchQuery(QObject* parent = nullptr)
        : QueryBase(parent),
          m_location(enums::SearchLocation::SearchLocal),
          m_type(enums::SearchType::SearchSong) {}

    auto location() -> enums::SearchLocation { return m_location; }
    void setLocation(enums::SearchLocation v) {
        if (ycore::cmp_exchange(m_location, v)) {
            locationChanged();
        }
    };

    Q_SIGNAL void locationChanged();

    auto type() -> enums::SearchType { return m_type; }
    void setType(enums::SearchType v) {
        if (ycore::cmp_exchange(m_type, v)) {
            typeChanged();
        };
    }

    Q_SIGNAL void typeChanged();

private:
    enums::SearchLocation m_location;
    enums::SearchType     m_type;
};

} // namespace qcm::query