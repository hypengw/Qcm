#include "Qcm/color.h"

#include "core/log.h"

#include "material_helper/helper.h"

using namespace qcm;

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

MdColorMgr::MdColorMgr(QObject* parent)
    : QObject(parent), m_accent_color(BASE_COLOR), m_scheme_theme(SchemeTheme::Light) {
    gen_scheme();
}

MdColorMgr::SchemeTheme MdColorMgr::schemeTheme() const { return m_scheme_theme; }
void                    MdColorMgr::set_schemeTheme(SchemeTheme v) {
    if (std::exchange(m_scheme_theme, v) != v) {
        gen_scheme();
        emit schemeThemeChanged();
    }
}

QColor MdColorMgr::accentColor() const { return m_accent_color; }

void MdColorMgr::set_accentColor(QColor v) {
    if (std::exchange(m_accent_color, v) != v) {
        gen_scheme();
        emit accentColorChanged();
    }
}

QColor MdColorMgr::getOn(QColor in) const {
    if (m_on_map.contains(in)) {
        return m_on_map.at(in);
    }
    return m_scheme.on_background;
}

void MdColorMgr::gen_scheme() {
    if (m_scheme_theme == SchemeTheme::Light)
        m_scheme = MaterialLightColorScheme(m_accent_color.rgb());
    else
        m_scheme = MaterialDarkColorScheme(m_accent_color.rgb());

    m_on_map = gen_on_map(m_scheme);
    emit schemeChanged();
}
