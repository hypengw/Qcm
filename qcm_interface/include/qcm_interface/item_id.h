#pragma once

#include <QObject>
#include <QUrl>

#include "core/core.h"

namespace qcm
{

namespace model
{

// itemid url
// itemid://{id_type}@{provider}/{id}
class ItemId {
    Q_GADGET
public:
    using validator_t = std::function<bool(const ItemId&)>;
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString sid READ id)
    Q_PROPERTY(QString provider READ provider)

    ItemId();
    ItemId(QStringView provider, QStringView type, QStringView id);
    ItemId(std::string_view provider, std::string_view type, std::string_view id);
    explicit ItemId(const QUrl&);
    ItemId(const QUrl&, validator_t);
    ~ItemId();
    ItemId(const ItemId&);
    ItemId& operator=(const ItemId&);
    ItemId(ItemId&&);
    ItemId& operator=(ItemId&&);

    auto type() const -> const QString&;
    auto id() const -> const QString&;
    auto provider() const -> const QString&;

    void                 set_validator(const validator_t&);
    std::strong_ordering operator<=>(const ItemId&) const;
    bool                 operator==(const ItemId&) const;

    Q_INVOKABLE bool valid() const;
    Q_INVOKABLE QUrl toUrl() const;

public Q_SLOTS:
    void set_type(QStringView);
    void set_id(QStringView);
    void set_provider(QStringView);

private:
    class Private;
    up<Private> d_ptr;
    C_DECLARE_PRIVATE(ItemId, d_ptr);
};
} // namespace model
} // namespace qcm

template<>
struct std::hash<qcm::model::ItemId> {
    std::size_t operator()(const qcm::model::ItemId& k) const;
};
