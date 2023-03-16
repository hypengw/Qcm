#include "Qcm/color.h"

#include "core/log.h"

#include "material_helper/helper.h"

using namespace qcm;

constexpr QRgb base_color { qRgb(190, 231, 253) };

namespace
{
std::map<QColor, QColor, QColorCompare> gen_on_map(const MdScheme& sh) {
    return { { sh.primary, sh.on_primary },
             { sh.secondary, sh.on_secondary },
             { sh.tertiary, sh.on_tertiary },
             { sh.error, sh.on_error },
             { sh.background, sh.on_background } };
}
} // namespace

MdColorMgr::MdColorMgr(QObject* parent): QObject(parent), m_scheme_theme(SchemeTheme::Light) {
    gen_scheme(SchemeTheme::Light);
}

MdColorMgr::SchemeTheme MdColorMgr::schemeTheme() const { return m_scheme_theme; }
void                    MdColorMgr::set_schemeTheme(SchemeTheme v) {
    if (std::exchange(m_scheme_theme, v) != v) {
        gen_scheme(v);
        emit schemeThemeChanged();
        emit schemeChanged();
    }
}

QColor MdColorMgr::getOn(QColor in) const {
    if (m_on_map.contains(in)) {
        return m_on_map.at(in);
    }
    return m_scheme.on_background;
}

void MdColorMgr::gen_scheme(SchemeTheme th) {
    if (th == SchemeTheme::Light)
        m_scheme = MaterialLightColorScheme(base_color);
    else
        m_scheme = MaterialDarkColorScheme(base_color);

    m_on_map = gen_on_map(m_scheme);
}
