#include "Qcm/model/item_id.hpp"
#include "core/qstr_helper.h"
#include "core/log.h"

namespace qcm::model
{

ItemId::ItemId(): m_type(enums::ItemType::ItemInvalid), m_library_id(-1), m_id(-1) {}
ItemId::ItemId(std::nullptr_t): ItemId() {}

ItemId::ItemId(enums::ItemType type, i64 id, i64 library_id): ItemId() {
    setType(type);
    setId(id);
    setLibraryId(library_id);
}

ItemId::ItemId(const QUrl& u): ItemId() { setUrl(u); }

ItemId& ItemId::operator=(const QUrl& url) {
    setUrl(url);
    return *this;
}

auto ItemId::type() const -> enums::ItemType { return m_type; }

auto ItemId::idStr() const -> QString { return QString::number(m_id); }

auto ItemId::libraryIdStr() const -> QString { return QString::number(m_library_id); }

auto ItemId::id() const -> i64 { return m_id; }
auto ItemId::pid() const -> QtProtobuf::int64 { return m_id; }

auto ItemId::libraryId() const -> i64 { return m_library_id; }

void ItemId::setType(enums::ItemType type) { m_type = type; }

void ItemId::setId(i64 id) { m_id = id; }

void ItemId::setLibraryId(i64 id) { m_library_id = id; }

void ItemId::setType(QStringView v) {
    m_type = rstd::from_str<enums::ItemType>(v.toString().toStdString()).unwrap();
}

void ItemId::setId(QStringView v) { m_id = v.toLongLong(); }

void ItemId::setLibraryId(QStringView v) { m_library_id = v.toLongLong(); }

void ItemId::setUrl(const QUrl& u) {
    _assert_(u.scheme() == "itemid" || u.scheme().isEmpty());
    setType(u.userName());
    setId(u.path().sliced(1));
    setLibraryId(u.password());
}

bool ItemId::valid() const { return m_type != enums::ItemType::ItemInvalid && m_id >= 0; }

QUrl ItemId::toUrl() const {
    QUrl url;
    url.setScheme("itemid");
    // url.setUserName(type());
    url.setPassword(libraryIdStr());
    url.setPath(QString("/%1").arg(m_id));
    return url;
}

std::strong_ordering ItemId::operator<=>(const ItemId& o) const noexcept {
    if (auto cmp = m_type <=> o.m_type; cmp != 0) return cmp;
    if (auto cmp = m_id <=> o.m_id; cmp != 0) return cmp;
    return m_library_id <=> o.m_library_id;
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

QUrl ItemId::toPageUrl() const {
    QUrl url;
    using ItemType = enums::ItemType;
    switch (type()) {
    case ItemType::ItemAlbum: {
        url = u"qrc:/Qcm/App/qml/page/detail/AlbumDetailPage.qml"_s;
        break;
    }
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
        log::warn("no page url for item type: {}", type());
    }
    }
    return url;
}

} // namespace qcm::model

std::size_t std::hash<qcm::model::ItemId>::operator()(const qcm::model::ItemId& k) const {
    usize s = 0;
    ycore::hash_combine(s, static_cast<int>(k.type()));
    ycore::hash_combine(s, k.id());
    ycore::hash_combine(s, k.libraryId());
    return s;
}

#include <Qcm/model/moc_item_id.cpp>