#include "Qcm/message/item_id.hpp"
#include <QString>
#include <QHashFunctions>

using namespace Qt::Literals::StringLiterals;

namespace qcm::model
{

ItemId::ItemId(): m_type(enums::ItemType::ItemInvalid), m_id(-1) {}
ItemId::ItemId(std::nullptr_t): ItemId() {}

ItemId::ItemId(enums::ItemType type, qint64 id): ItemId() {
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

auto ItemId::id() const -> qint64 { return m_id; }
auto ItemId::pid() const -> QtProtobuf::int64 { return m_id; }

void ItemId::setType(enums::ItemType type) { m_type = type; }

void ItemId::setId(qint64 id) { m_id = id; }

void ItemId::setType(QStringView v) {
    m_type = enums::item_type_from_str(v.toString().toStdString());
}

void ItemId::setId(QStringView v) { m_id = v.toLongLong(); }

void ItemId::setUrl(const QUrl& u) {
    // debug_assert(u.scheme() == "itemid" || u.scheme().isEmpty());
    setType(u.userName());
    setId(u.path().sliced(1));
}

bool ItemId::valid() const { return m_type != enums::ItemType::ItemInvalid && m_id >= 0; }

QUrl ItemId::toUrl() const {
    QUrl url;
    url.setScheme("itemid");
    url.setUserName(QString::fromUtf8(enums::item_type_to_str(m_type)));
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
    QUrl url(QString::fromUtf8(b));
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
        // LOG_WARN("no page url for item type: {}", type());
    }
    }
    return url;
}

} // namespace qcm::model

std::size_t std::hash<qcm::model::ItemId>::operator()(const qcm::model::ItemId& k) const noexcept {
    std::size_t s = 0;
    return qHashMulti(s, static_cast<int>(k.type()), k.id());
}

namespace qcm
{
auto enums::item_type_to_str(ItemType t) -> std::string_view {
    std::string_view name;
    switch (t) {
    case ItemType::ItemInvalid: name = "Invalid"; break;
    case ItemType::ItemProvider: name = "Provider"; break;
    case ItemType::ItemLibrary: name = "Library"; break;
    case ItemType::ItemAlbum: name = "Album"; break;
    case ItemType::ItemAlbumArtist: name = "AlbumArtist"; break;
    case ItemType::ItemArtist: name = "Artist"; break;
    case ItemType::ItemMix: name = "Mix"; break;
    case ItemType::ItemRadio: name = "Radio"; break;
    case ItemType::ItemRadioQueue: name = "RadioQueue"; break;
    case ItemType::ItemSong: name = "Song"; break;
    case ItemType::ItemProgram: name = "Program"; break;
    }
    return name;
}

auto enums::item_type_from_str(std::string_view str) -> ItemType {
    using Self = ItemType;
    if (str == "Provider") return Self::ItemProvider;
    if (str == "Library") return Self::ItemLibrary;
    if (str == "Album") return Self::ItemAlbum;
    if (str == "AlbumArtist") return Self::ItemAlbumArtist;
    if (str == "Artist") return Self::ItemArtist;
    if (str == "Mix") return Self::ItemMix;
    if (str == "Radio") return Self::ItemRadio;
    if (str == "RadioQueue") return Self::ItemRadioQueue;
    if (str == "Song") return Self::ItemSong;
    if (str == "Program") return Self::ItemProgram;
    return Self::ItemInvalid;
}
} // namespace qcm

#include "Qcm/message/moc_item_id.cpp"