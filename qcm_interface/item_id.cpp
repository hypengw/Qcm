#include "qcm_interface/item_id.h"

#include "core/qstr_helper.h"
#include "core/log.h"

namespace qcm::model
{

class ItemId::Private {
public:
    QString     id;
    QString     provider;
    QString     type;
    validator_t validtor;
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

ItemId::ItemId(const QUrl& u): ItemId() {
    C_D(ItemId);
    _assert_(u.scheme() == "itemid");
    auto p = u.path();
    // remove '/'
    _assert_(p.startsWith('/'));
    d->id       = p.isEmpty() ? p : p.sliced(1);
    d->provider = u.host();
    d->type     = u.userName();
}
ItemId::ItemId(const QUrl& url, validator_t v): ItemId(url) {
    C_D(ItemId);
    set_validator(v);
}
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
    return ! (id().isEmpty() || provider().isEmpty()) && (d->validtor ? d->validtor(*this) : true);
}

void ItemId::set_validator(const validator_t& v) {
    C_D(ItemId);
    std::exchange(d->validtor, v);
}
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

QUrl ItemId::toUrl() const {
    QUrl url;
    url.setScheme("itemid");
    url.setUserName(type());
    url.setHost(provider());
    url.setPath(convert_from<QString>(fmt::format("/{}", id())));
    return url;
}

std::strong_ordering ItemId::operator<=>(const ItemId& o) const {
    C_D(const ItemId);
    std::strong_ordering cmp { std::strong_ordering::equal };
    do {
        if (cmp = d->provider <=> o.d_func()->provider; cmp != 0) break;
        if (cmp = d->type <=> o.d_func()->type; cmp != 0) break;
        if (cmp = d->id <=> o.d_func()->id; cmp != 0) break;
    } while (false);
    return cmp;
}

bool ItemId::operator==(const ItemId& o) const {
    return (*this <=> o) == std::strong_ordering::equal;
}
} // namespace qcm::model

std::size_t std::hash<qcm::model::ItemId>::operator()(const qcm::model::ItemId& k) const {
    usize s = 0;
    ycore::hash_combine(s, k.provider());
    ycore::hash_combine(s, k.type());
    ycore::hash_combine(s, k.id());
    return s;
}