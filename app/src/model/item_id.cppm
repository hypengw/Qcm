module;
#include <QObject>
#include <QUrl>
#include <QQmlEngine>
#include <QtProtobuf/qtprotobuftypes.h>

#include "core/log.h"
#include "core/qstr_helper.h"
#include "Qcm/model/item_id.moc.h"

#ifdef Q_MOC_RUN
#include "Qcm/model/item_id.moc"
#endif
export module qcm.model.item_id;
export import qcm.qml.enums;
export import qcm.core;

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
    bool                 operator==(std::string_view) const;
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
struct std::hash<qcm::model::ItemId> {
    std::size_t operator()(const qcm::model::ItemId& k) const noexcept;
};


module :private;

namespace qcm::model
{

ItemId::ItemId(): m_type(enums::ItemType::ItemInvalid), m_id(-1) {}
ItemId::ItemId(std::nullptr_t): ItemId() {}

ItemId::ItemId(enums::ItemType type, i64 id): ItemId() {
    setType(type);
    setId(id);
}

ItemId::ItemId(const QUrl& u): ItemId() { setUrl(u); }

ItemId& ItemId::operator=(const QUrl& url) {
    setUrl(url);
    return *this;
}

auto ItemId::type() const -> enums::ItemType { return m_type; }

auto ItemId::idStr() const -> QString { return QString::number(m_id); }

auto ItemId::id() const -> i64 { return m_id; }
auto ItemId::pid() const -> QtProtobuf::int64 { return m_id; }

void ItemId::setType(enums::ItemType type) { m_type = type; }

void ItemId::setId(i64 id) { m_id = id; }

void ItemId::setType(QStringView v) {
    m_type = rstd::from_str<enums::ItemType>(v.toString().toStdString()).unwrap();
}

void ItemId::setId(QStringView v) { m_id = v.toLongLong(); }

void ItemId::setUrl(const QUrl& u) {
    _assert_(u.scheme() == "itemid" || u.scheme().isEmpty());
    setType(u.userName());
    setId(u.path().sliced(1));
}

bool ItemId::valid() const { return m_type != enums::ItemType::ItemInvalid && m_id >= 0; }

QUrl ItemId::toUrl() const {
    QUrl url;
    url.setScheme("itemid");
    url.setUserName(QString::fromUtf8(std::format("{}", type())));
    url.setPath(QString("/%1").arg(m_id));
    return url;
}

std::strong_ordering ItemId::operator<=>(const ItemId& o) const noexcept {
    if (auto cmp = m_type <=> o.m_type; cmp != 0) return cmp;
    return m_id <=> o.m_id;
}

bool ItemId::operator==(const ItemId& o) const noexcept {
    return (*this <=> o) == std::strong_ordering::equal;
}

bool ItemId::operator==(const QUrl& url) const {
    ItemId o(url);
    return o == *this;
}

bool ItemId::operator==(std::string_view b) const {
    QUrl url(convert_from<QString>(b));
    return *this == url;
}

bool ItemId::operator==(QStringView b) const {
    QUrl url(b.toString());
    return *this == url;
}

ItemId ItemId::clone() const noexcept { return *this; }

QUrl ItemId::toPageUrl() const {
    QUrl url;
    using ItemType = enums::ItemType;
    switch (type()) {
    case ItemType::ItemAlbum: {
        url = u"qrc:/Qcm/App/qml/page/detail/AlbumDetailPage.qml"_s;
        break;
    }
    case ItemType::ItemAlbumArtist:
    case ItemType::ItemArtist: {
        url = u"qrc:/Qcm/App/qml/page/detail/ArtistDetailPage.qml"_s;
        break;
    }
    case ItemType::ItemMix: {
        url = u"qrc:/Qcm/App/qml/page/detail/MixDetailPage.qml"_s;
        break;
    }
    case ItemType::ItemRadio: {
        url = u"qrc:/Qcm/App/qml/page/detail/RadioDetailPage.qml"_s;
        break;
    }
    default: {
        LOG_WARN("no page url for item type: {}", type());
    }
    }
    return url;
}

} // namespace qcm::model

std::size_t std::hash<qcm::model::ItemId>::operator()(const qcm::model::ItemId& k) const noexcept {
    usize s = 0;
    ycore::hash_combine(s, static_cast<int>(k.type()));
    ycore::hash_combine(s, k.id());
    return s;
}

#include "Qcm/model/item_id.moc.cpp"