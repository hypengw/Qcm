#include "qml_material/color.h"

#include <QtGui/QGuiApplication>
#include <QStyleHints>

#include "core/log.h"
#include "qml_material/helper.h"
#include "qml_material/util.h"

using namespace qcm;
using namespace qml_material;

namespace
{
constexpr QRgb BASE_COLOR { qRgb(190, 231, 253) };

std::map<QColor, QColor, QColorCompare> gen_on_map(const MdScheme& sh) {
    return {
        { sh.primary, sh.on_primary },
        { sh.primary_container, sh.on_primary_container },
        { sh.secondary, sh.on_secondary },
        { sh.secondary_container, sh.on_secondary_container },
        { sh.tertiary, sh.on_tertiary },
        { sh.tertiary_container, sh.on_tertiary_container },
        { sh.error, sh.on_error },
        { sh.background, sh.on_background },
        { sh.inverse_surface, sh.inverse_on_surface },
        { sh.surface, sh.on_surface },
        { sh.surface_variant, sh.on_surface_variant },
        { sh.surface_1, sh.on_surface },
        { sh.surface_2, sh.on_surface },
        { sh.surface_3, sh.on_surface },
        { sh.surface_4, sh.on_surface },
        { sh.surface_5, sh.on_surface },
        { sh.surface_dim, sh.on_surface },
        { sh.surface_bright, sh.on_surface },
        { sh.surface_container, sh.on_surface },
        { sh.surface_container_high, sh.on_surface },
        { sh.surface_container_highest, sh.on_surface },
        { sh.surface_container_low, sh.on_surface },
        { sh.surface_container_lowest, sh.on_surface },
    };
}
} // namespace

DEFINE_CONVERT(qml_material::MdColorMgr::ColorSchemeEnum, Qt::ColorScheme) {
    switch (in) {
    case in_type::Dark: out = out_type::Dark; break;
    default: out = out_type::Light;
    }
}

MdColorMgr::MdColorMgr(QObject* parent)
    : QObject(parent),
      m_accent_color(BASE_COLOR),
      m_color_scheme(sysColorScheme()),
      m_use_sys_color_scheme(true),
      m_use_sys_accent_color(false) {
    gen_scheme();
    connect(this, &Self::colorSchemeChanged, this, &Self::gen_scheme);
    connect(this, &Self::accentColorChanged, this, &Self::gen_scheme);

    connect(Xdp::insance(), &Xdp::colorSchemeChanged, this, &Self::refrehFromSystem);
    connect(Xdp::insance(), &Xdp::accentColorChanged, this, &Self::refrehFromSystem);
    connect(this, &Self::useSysColorSMChanged, this, &Self::refrehFromSystem);
    connect(this, &Self::useSysAccentColorChanged, this, &Self::refrehFromSystem);
}

MdColorMgr::ColorSchemeEnum MdColorMgr::sysColorScheme() const {
    return convert_from<ColorSchemeEnum>(Xdp::insance()->colorScheme());
}
QColor MdColorMgr::sysAccentColor() const { return Xdp::insance()->accentColor(); }

MdColorMgr::ColorSchemeEnum MdColorMgr::colorScheme() const { return m_color_scheme; }
void                        MdColorMgr::set_colorScheme(ColorSchemeEnum v) {
    if (std::exchange(m_color_scheme, v) != v) {
        emit colorSchemeChanged();
    }
}

QColor MdColorMgr::accentColor() const { return m_accent_color; }

bool MdColorMgr::useSysColorSM() const { return m_use_sys_color_scheme; };
bool MdColorMgr::useSysAccentColor() const { return m_use_sys_accent_color; };

void MdColorMgr::set_accentColor(QColor v) {
    if (std::exchange(m_accent_color, v) != v) {
        emit accentColorChanged();
    }
}

void MdColorMgr::set_useSysColorSM(bool v) {
    if (std::exchange(m_use_sys_color_scheme, v) != v) {
        emit useSysColorSMChanged();
    }
}

void MdColorMgr::set_useSysAccentColor(bool v) {
    if (std::exchange(m_use_sys_accent_color, v) != v) {
        emit useSysAccentColorChanged();
    }
}

QColor MdColorMgr::getOn(QColor in) const {
    if (m_on_map.contains(in)) {
        return m_on_map.at(in);
    }
    return m_scheme.on_background;
}

void MdColorMgr::gen_scheme() {
    auto cs = colorScheme();
    if (cs == ColorSchemeEnum::Light)
        m_scheme = MaterialLightColorScheme(m_accent_color.rgb());
    else
        m_scheme = MaterialDarkColorScheme(m_accent_color.rgb());

    m_on_map = gen_on_map(m_scheme);
    emit schemeChanged();
}

void MdColorMgr::refrehFromSystem() {
    if (useSysColorSM()) {
        set_colorScheme(sysColorScheme());
    }

    if (useSysAccentColor()) {
        set_accentColor(sysAccentColor());
    }
}
