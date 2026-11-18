module;
#include <QByteArray>
#include <QGuiApplication>
#include <QString>
#include <QStringView>
#include <QWindow>

#if defined(Q_OS_LINUX) && QT_CONFIG(xcb)
#    include <QtGui/private/qtx11extras_p.h>
#endif

module qcm.activation;

using namespace Qt::Literals::StringLiterals;

namespace qcm::activation
{
auto take_token() -> QString {
    auto token      = qgetenv("XDG_ACTIVATION_TOKEN");
    auto startup_id = qgetenv("DESKTOP_STARTUP_ID");
    qunsetenv("XDG_ACTIVATION_TOKEN");
    qunsetenv("DESKTOP_STARTUP_ID");
    return QString::fromUtf8(token.isEmpty() ? startup_id : token);
}

WindowActivation::WindowActivation(QStringView token) {
    if (token.isEmpty()) return;
    const auto platform = QGuiApplication::platformName();
    if (platform.startsWith(u"wayland"_s)) {
        // Qt's xdg-shell consumes this token when showing or activating the window.
        m_wayland_token = qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
    }
#if defined(Q_OS_LINUX) && QT_CONFIG(xcb)
    else if (platform == u"xcb"_s) {
        QX11Info::setNextStartupId(token.toUtf8());
        m_x11_token = true;
    }
#endif
}

WindowActivation::~WindowActivation() {
    if (m_wayland_token) qunsetenv("XDG_ACTIVATION_TOKEN");
#if defined(Q_OS_LINUX) && QT_CONFIG(xcb)
    if (m_x11_token) QX11Info::setNextStartupId({});
#endif
}

void WindowActivation::activate(QWindow* window) const {
    if (! window) return;
    window->setWindowStates(window->windowStates() & ~Qt::WindowMinimized);
    window->show();
    window->raise();
    window->requestActivate();
}
} // namespace qcm::activation
