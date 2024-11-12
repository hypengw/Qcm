#include "qcm_interface/item_id.h"

#include "core/qstr_helper.h"
#include "core/log.h"
#include "qcm_interface/plugin.h"

namespace qcm::model
{

class ItemId::Private {
public:
    QString id;
    QString provider;
    QString type;
};

ItemId::ItemId(): d_ptr(make_up<Private>()) {}
ItemId::ItemId(std::nullptr_t): ItemId() {}

ItemId::ItemId(QStringView provider, QStringView type, QStringView id): ItemId() {
    set_provider(provider);
    set_type(type);
    set_id(id);
}

ItemId::ItemId(std::string_view provider, std::string_view type, std::string_view id): ItemId() {
    set_provider(convert_from<QString>(provider));
    set_type(convert_from<QString>(type));
    set_id(convert_from<QString>(id));
}

ItemId::ItemId(const QUrl& u): ItemId() { set_url(u); }
ItemId::~ItemId() {}
ItemId::ItemId(const ItemId& o): ItemId() { *this = o; }
ItemId& ItemId::operator=(const ItemId& o) {
    C_D(ItemId);
    *(d) = *(o.d_func());
    return *this;
}
ItemId::ItemId(ItemId&& o): ItemId() { d_ptr.swap(o.d_ptr); }
ItemId& ItemId::operator=(ItemId&& o) {
    d_ptr.swap(o.d_ptr);
    return *this;
}

ItemId& ItemId::operator=(const QUrl& url) {
    set_url(url);
    return *this;
}

auto ItemId::type() const -> const QString& {
    C_D(const ItemId);
    return d->type;
}
auto ItemId::id() const -> const QString& {
    C_D(const ItemId);
    return d->id;
}
auto ItemId::provider() const -> const QString& {
    C_D(const ItemId);
    return d->provider;
}
auto ItemId::valid() const -> bool {
    C_D(const ItemId);

    if (id().isEmpty() || provider().isEmpty() || id() == "invalid" || id() == "empty") {
        return false;
    }
    if (auto p = PluginManager::instance()->plugin(provider().toStdString())) {
        return p->get().valid_id(*this);
    }
    return true;
}

void ItemId::set_type(std::string_view v) { set_type(convert_from<QString>(v)); }
void ItemId::set_id(std::string_view v) { set_id(convert_from<QString>(v)); }
void ItemId::set_provider(std::string_view v) { set_provider(convert_from<QString>(v)); }

void ItemId::set_type(QStringView v) {
    C_D(ItemId);
    std::exchange(d->type, v.toString());
}
void ItemId::set_id(QStringView v) {
    C_D(ItemId);
    std::exchange(d->id, v.toString());
}
void ItemId::set_provider(QStringView v) {
    C_D(ItemId);
    std::exchange(d->provider, v.toString());
}

void ItemId::set_url(const QUrl& u) {
    C_D(ItemId);
    _assert_(u.scheme() == "itemid" || u.scheme().isEmpty());
    auto p = u.path();
    // remove '/'
    _assert_(p.startsWith('/') || p.isEmpty());
    d->id       = p.isEmpty() ? p : p.sliced(1);
    d->provider = u.host();
    d->type     = u.userName();
}

QUrl ItemId::toUrl() const {
    QUrl url;
    url.setScheme("itemid");
    url.setUserName(type());
    url.setHost(provider());
    url.setPath(convert_from<QString>(fmt::format("/{}", id())));
    return url;
}

std::strong_ordering ItemId::operator<=>(const ItemId& o) const noexcept {
    C_D(const ItemId);
    std::strong_ordering cmp { std::strong_ordering::equal };
    do {
        if (cmp = d->provider <=> o.d_func()->provider; cmp != 0) break;
        if (cmp = d->type <=> o.d_func()->type; cmp != 0) break;
        if (cmp = d->id <=> o.d_func()->id; cmp != 0) break;
    } while (false);
    return cmp;
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
    QUrl url(convert_from<QString>(b));
    return *this == url;
}

} // namespace qcm::model

std::size_t std::hash<qcm::model::ItemId>::operator()(const qcm::model::ItemId& k) const {
    usize s = 0;
    ycore::hash_combine(s, k.provider());
    ycore::hash_combine(s, k.type());
    ycore::hash_combine(s, k.id());
    return s;
}