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
// itemid://{id_type}@{provider}/{id}
class QCM_INTERFACE_API ItemId {
    Q_GADGET
    QML_VALUE_TYPE(t_id)
public:
    using validator_t = std::function<bool(const ItemId&)>;
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString sid READ id)
    Q_PROPERTY(QString provider READ provider)

    ItemId();
    ItemId(std::nullptr_t);
    ItemId(QStringView provider, QStringView type, QStringView id);
    ItemId(std::string_view provider, std::string_view type, std::string_view id);
    explicit ItemId(const QUrl&);
    ItemId(const QUrl&, validator_t);
    ~ItemId();
    ItemId(const ItemId&);
    ItemId& operator=(const ItemId&);
    ItemId(ItemId&&);
    ItemId& operator=(ItemId&&);

    ItemId& operator=(const QUrl&);

    auto type() const -> const QString&;
    auto id() const -> const QString&;
    auto provider() const -> const QString&;

    void                 set_validator(const validator_t&);
    std::strong_ordering operator<=>(const ItemId&) const;
    bool                 operator==(const ItemId&) const;
    bool                 operator==(const QUrl&) const;
    bool                 operator==(std::string_view) const;
    bool                 operator==(QStringView) const;

    Q_INVOKABLE bool valid() const;
    Q_INVOKABLE QUrl toUrl() const;

    void set_type(std::string_view);
    void set_id(std::string_view);
    void set_provider(std::string_view);

public Q_SLOTS:
    void set_type(QStringView);
    void set_id(QStringView);
    void set_provider(QStringView);
    void set_url(const QUrl&);

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
