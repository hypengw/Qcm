#pragma once

#include <QQmlEngine>
#include <QtGui/qcolor.h>

#include <unordered_map>

#include "core/core.h"
#include "material_helper/helper.h"

namespace qcm
{

struct QColorCompare {
    using is_transparent = void;
    bool operator()(const QColor& a, const QColor& b) const noexcept { return a.rgba() < b.rgba(); }
};

class MdColorMgr : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    MdColorMgr(QObject* = nullptr);

    enum SchemeTheme
    {
        Light,
        Dark
    };
    Q_ENUM(SchemeTheme)

    Q_PROPERTY(
        SchemeTheme schemeTheme READ schemeTheme WRITE set_schemeTheme NOTIFY schemeThemeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE set_accentColor NOTIFY accentColorChanged)

#define X(_n_)                                           \
    Q_PROPERTY(QColor _n_ READ _n_ NOTIFY schemeChanged) \
    QColor _n_() const { return m_scheme._n_; }

    X(primary)
    X(on_primary)
    X(primary_container)
    X(on_primary_container)
    X(secondary)
    X(on_secondary)
    X(secondary_container)
    X(on_secondary_container)
    X(tertiary)
    X(on_tertiary)
    X(tertiary_container)
    X(on_tertiary_container)
    X(error)
    X(on_error)
    X(error_container)
    X(on_error_container)
    X(background)
    X(on_background)
    X(surface)
    X(on_surface)
    X(surface_variant)
    X(on_surface_variant)
    X(outline)
    X(outline_variant)
    X(shadow)
    X(scrim)
    X(inverse_surface)
    X(inverse_on_surface)
    X(inverse_primary)
    X(surface_1)
    X(surface_2)
    X(surface_3)
    X(surface_4)
    X(surface_5)
    X(surface_dim)
    X(surface_bright)
    X(surface_container)
    X(surface_container_low)
    X(surface_container_lowest)
    X(surface_container_high)
    X(surface_container_highest)
#undef X

public:
    SchemeTheme schemeTheme() const;
    QColor      accentColor() const;

    Q_INVOKABLE QColor getOn(QColor) const;

public slots:
    void set_schemeTheme(SchemeTheme);
    void set_accentColor(QColor);

signals:
    void schemeThemeChanged();
    void schemeChanged();
    void accentColorChanged();

private:
    void gen_scheme();

    QColor                                  m_accent_color;
    SchemeTheme                             m_scheme_theme;
    MdScheme                                m_scheme;
    std::map<QColor, QColor, QColorCompare> m_on_map;
};

} // namespace qcm
