#include "qml_material/util.h"

#include <set>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QGuiApplication>
#include <QtGui/qpa/qplatformtheme.h>
#include <QStyleHints>
#include <QGlobalStatic>
#include "core/log.h"
#include "core/qstr_helper.h"

Q_GLOBAL_STATIC(qml_material::Xdp, TheXdp)

namespace
{
inline constexpr auto kService           = "org.freedesktop.portal.Desktop";
inline constexpr auto kObjectPath        = "/org/freedesktop/portal/desktop";
inline constexpr auto kRequestInterface  = "org.freedesktop.portal.Request";
inline constexpr auto kSettingsInterface = "org.freedesktop.portal.Settings";

Qt::ColorScheme to_color_scheme(const QVariant& in) {
    auto v = in.toUInt();
    return v == 1 ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

QColor to_accent_color(const QVariant& in) {
    std::array<double, 3> c { 0 };
    const auto            v = in.value<QDBusArgument>();
    v.beginStructure();
    v >> c[0] >> c[1] >> c[2];
    v.endStructure();
    return QColor::fromRgbF(c[0], c[1], c[2]);
}
} // namespace

namespace qml_material
{
Xdp::Xdp(QObject* parent): QObject(parent) {
    auto bus = QDBusConnection::sessionBus();
    auto res = bus.connect(kService,
                           kObjectPath,
                           kSettingsInterface,
                           "SettingChanged",
                           this,
                           SLOT(xdpSettingChangeSlot(QString, QString, QDBusVariant)));
    _assert_(res);

    auto message =
        QDBusMessage::createMethodCall(kService, kObjectPath, kSettingsInterface, "Read");

    {
        message << "org.freedesktop.appearance"
                << "color-scheme";
        // this must not be asyncCall() because we have to set appearance now
        QDBusReply<QVariant> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(reply.value());
            m_color_scheme                 = to_color_scheme(dbusVariant.variant());
        }
    }

    {
        message.setArguments({ "org.freedesktop.appearance", "accent-color" });
        QDBusReply<QVariant> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const QDBusVariant dbusVariant = qvariant_cast<QDBusVariant>(reply.value());
            m_accent_color                 = to_accent_color(dbusVariant.variant());
        }
    }
}
Xdp::~Xdp() {}

Xdp* Xdp::insance() { return TheXdp; }

void Xdp::xdpSettingChangeSlot(QString namespace_, QString key, QDBusVariant value_) {
    auto value = value_.variant();
    if (namespace_ == "org.freedesktop.appearance" && key == "color-scheme") {
        m_color_scheme = to_color_scheme(value);
        Q_EMIT colorSchemeChanged();
    } else if (namespace_ == "org.freedesktop.appearance" && key == "accent-color") {
        m_accent_color = to_accent_color(value);
        Q_EMIT accentColorChanged();
    }

    DEBUG_LOG("xdp SettingChanged: {} {}, v({}): {}",
              namespace_,
              key,
              value.metaType().name(),
              value.toString());
}

QColor Xdp::accentColor() const { return m_accent_color.value_or(QColor {}); }

Qt::ColorScheme Xdp::colorScheme() const {
    return m_color_scheme ? m_color_scheme.value() : QGuiApplication::styleHints()->colorScheme();
}

Util::Util(QObject* parent): QObject(parent) {}
Util::~Util() {}

CornersGroup Util::corner(QVariant in) {
    CornersGroup out;
    if (in.canConvert<qreal>()) {
        out = CornersGroup(in.value<qreal>());
    } else if (auto list = in.toList(); ! list.empty()) {
        switch (list.size()) {
        case 1: {
            out = CornersGroup(list[0].value<qreal>());
            break;
        }
        case 2: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[0].value<qreal>());
            out.setBottomLeft(list[1].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        case 3: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[1].value<qreal>());
            break;
        }
        default:
        case 4: {
            out.setTopLeft(list[0].value<qreal>());
            out.setTopRight(list[1].value<qreal>());
            out.setBottomLeft(list[2].value<qreal>());
            out.setBottomRight(list[3].value<qreal>());
        }
        }
    }
    return out;
}

CornersGroup Util::corner(qreal br, qreal tr, qreal bl, qreal tl) {
    return CornersGroup(br, tr, bl, tl);
}

void Util::track(QVariant, Track t) {
    switch (t) {
    case TrackCreate:
        m_tracked++;
        WARN_LOG("track create {}", m_tracked);
        break;
    case TrackDelete:
        m_tracked--;
        WARN_LOG("track delete {}", m_tracked);
        break;
    }
}

} // namespace qml_material