#pragma once

#include <QObject>
#include <QUrl>
#include <QQmlEngine>

#include "core/core.h"
#include "qcm_interface/export.h"

namespace qcm
{

namespace model
{

// itemid url
// itemid://{id_type}:{library_id}@{provider}/{id}
class QCM_INTERFACE_API ItemId {
    Q_GADGET
    QML_VALUE_TYPE(t_id)
public:
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString sid READ id)
    Q_PROPERTY(QString provider READ provider)
    Q_PROPERTY(QString libraryId READ library_id_str)

    ItemId();
    ItemId(std::nullptr_t);
    ItemId(QStringView provider, QStringView type, QStringView id, i64 library_id);
    ItemId(QStringView type, QStringView id);
    ItemId(std::string_view provider, std::string_view type, std::string_view id, i64 library_id);
    explicit ItemId(const QUrl&);
    ~ItemId();
    ItemId(const ItemId&);
    ItemId& operator=(const ItemId&);
    ItemId(ItemId&&);
    ItemId& operator=(ItemId&&);

    ItemId& operator=(const QUrl&);

    auto type() const -> const QString&;
    auto id() const -> const QString&;
    auto provider() const -> const QString&;
    auto library_id_str() const -> QString;
    auto library_id() const -> i64;

    std::strong_ordering operator<=>(const ItemId&) const noexcept;
    bool                 operator==(const ItemId&) const noexcept;
    bool                 operator==(const QUrl&) const;
    bool                 operator==(std::string_view) const;
    bool                 operator==(QStringView) const;

    Q_INVOKABLE bool valid() const;
    Q_INVOKABLE QUrl toUrl() const;

    void set_type(std::string_view);
    void set_id(std::string_view);
    void set_provider(std::string_view);

    Q_SLOT void set_type(QStringView);
    Q_SLOT void set_id(QStringView);
    Q_SLOT void set_provider(QStringView);
    Q_SLOT void set_url(const QUrl&);
    Q_SLOT void set_library_id(QStringView);
    void        set_library_id(i64);

private:
    class Private;
    C_DECLARE_PRIVATE(ItemId, d_ptr);
};
} // namespace model
} // namespace qcm

template<>
struct QCM_INTERFACE_API std::hash<qcm::model::ItemId> {
    std::size_t operator()(const qcm::model::ItemId& k) const;
};
