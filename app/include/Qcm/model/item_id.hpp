#pragma once

#include <QObject>
#include <QUrl>
#include <QQmlEngine>

#include "core/core.h"
#include "Qcm/qml/enum.hpp"

namespace qcm::model
{

// itemid url
// itemid://{id_type}:{library_id}/{id}
class ItemId {
    Q_GADGET
    QML_VALUE_TYPE(item_id)
public:
    Q_PROPERTY(qcm::enums::ItemType type READ type)
    Q_PROPERTY(QString sid READ idStr)
    Q_PROPERTY(QString libraryId READ libraryIdStr)

    ItemId();
    ItemId(std::nullptr_t);
    ItemId(enums::ItemType, i64 id, i64 library_id = -1);

    explicit ItemId(const QUrl&);
    ItemId(const ItemId&)                = default;
    ItemId& operator=(const ItemId&)     = default;
    ItemId(ItemId&&) noexcept            = default;
    ItemId& operator=(ItemId&&) noexcept = default;

    ItemId& operator=(const QUrl&);

    auto type() const -> enums::ItemType;
    auto idStr() const -> const QString&;
    auto libraryIdStr() const -> QString;

    auto id() const -> i64;
    auto libraryId() const -> i64;

    void setType(enums::ItemType);
    void setId(i64);
    void setLibraryId(i64);

    void setType(QStringView);
    void setId(QStringView);
    void setUrl(const QUrl&);
    void setLibraryId(QStringView);

    std::strong_ordering operator<=>(const ItemId&) const noexcept;
    bool                 operator==(const ItemId&) const noexcept;
    bool                 operator==(const QUrl&) const;
    bool                 operator==(std::string_view) const;
    bool                 operator==(QStringView) const;

    Q_INVOKABLE bool valid() const;
    Q_INVOKABLE QUrl toUrl() const;

    Q_INVOKABLE QUrl toPageUrl() const;

private:
    enums::ItemType m_type;
    i64             m_library_id;
    i64             m_id;
};
} // namespace qcm::model

template<>
struct std::hash<qcm::model::ItemId> {
    std::size_t operator()(const qcm::model::ItemId& k) const;
};
