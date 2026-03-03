module;
#include "Qcm/macro_qt.hpp"
#include "core/log.h"

#ifdef Q_MOC_RUN
#include "Qcm/model/item_id.moc"
#endif
export module qcm:model.item_id;
export import :qml.enums;
export import qcm.qt;

export namespace qcm::model
{

// itemid url
// itemid://{id_type}/{id}
class ItemId {
    Q_GADGET
    QML_VALUE_TYPE(item_id)
    Q_PROPERTY(QtProtobuf::int64 id READ pid)
    Q_PROPERTY(qcm::enums::ItemType type READ type WRITE setType)
    Q_PROPERTY(QString sid READ idStr)
    Q_PROPERTY(bool valid READ valid)
    Q_PROPERTY(QUrl url READ toUrl)
    Q_PROPERTY(QUrl pageUrl READ toPageUrl)
public:
    ItemId();
    ItemId(std::nullptr_t);
    ItemId(enums::ItemType, i64 id);

    explicit ItemId(const QUrl&);
    ItemId(const ItemId&)                = default;
    ItemId& operator=(const ItemId&)     = default;
    ItemId(ItemId&&) noexcept            = default;
    ItemId& operator=(ItemId&&) noexcept = default;

    ItemId& operator=(const QUrl&);

    auto type() const -> enums::ItemType;
    auto idStr() const -> QString;

    auto id() const -> i64;
    auto pid() const -> QtProtobuf::int64;

    void setType(enums::ItemType);
    void setId(i64);

    void setType(QStringView);
    void setId(QStringView);
    void setUrl(const QUrl&);

    std::strong_ordering operator<=>(const ItemId&) const noexcept;
    bool                 operator==(const ItemId&) const noexcept;
    bool                 operator==(const QUrl&) const;
    bool                 operator==(rstd::cppstd::string_view) const;
    bool                 operator==(QStringView) const;

    bool valid() const;
    auto toUrl() const -> QUrl;

    auto toPageUrl() const -> QUrl;

    Q_INVOKABLE ItemId clone() const noexcept;

    QString toString() const { return toUrl().toString(); }

private:
    enums::ItemType m_type;
    i64             m_id;
};
} // namespace qcm::model

template<>
struct rstd::cppstd::hash<qcm::model::ItemId> {
    usize operator()(const qcm::model::ItemId& k) const noexcept;
};


