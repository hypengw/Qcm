#include "Qcm/qml_util.h"

namespace qcm::qml
{
auto Util::create_page() const -> model::Page { return {}; }
auto Util::create_itemid() const -> model::ItemId { return {}; }

auto Util::mpris_trackid(model::ItemId id) const -> QString {
    static const auto dbus_path = QString(APP_ID).replace('.', '/');
    auto              provider  = id.provider();
    auto              sid       = id.id();
    return QString("/%1/TrackId/%2/%3")
        .arg(dbus_path)
        .arg(provider.isEmpty() ? u"unknown"_qs : provider)
        .arg(sid.isEmpty() ? u"0"_qs : sid);
}

} // namespace qcm::qml