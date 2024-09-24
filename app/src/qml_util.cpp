#include "Qcm/qml_util.h"
#include "meta_model/qgadget_helper.h"

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

auto Util::create_route_msg(QVariantMap props) const -> model::RouteMsg {
    model::RouteMsg msg;
    msg.set_url(props.value("url").toUrl());
    msg.set_props(props.value("props").toMap());
    return msg;
}
auto Util::create_playlist(const QJSValue& js) const -> model::Playlist {
    return meta_model::toGadget<model::Playlist>(js);
}
auto Util::create_album(const QJSValue& js) const -> model::Album {
    return meta_model::toGadget<model::Album>(js);
}
auto Util::create_song(const QJSValue& js) const -> model::Song {
    return meta_model::toGadget<model::Song>(js);
}
auto Util::create_artist(const QJSValue& js) const -> model::Artist {
    return meta_model::toGadget<model::Artist>(js);
}
auto Util::create_djradio(const QJSValue& js) const -> model::Djradio {
    return meta_model::toGadget<model::Djradio>(js);
}
auto Util::create_program(const QJSValue& js) const -> model::Program {
    return meta_model::toGadget<model::Program>(js);
}
} // namespace qcm::qml