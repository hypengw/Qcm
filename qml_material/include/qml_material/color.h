#pragma once

#include <unordered_map>

#include <QtQml/QQmlEngine>
#include <QtGui/QColor>

#include "core/core.h"
#include "qml_material/helper.h"

namespace qml_material
{

struct QColorCompare {
    using is_transparent = void;
    bool operator()(const QColor& a, const QColor& b) const noexcept { return a.rgba() < b.rgba(); }
};

class MdColorMgr : public QObject {
    Q_OBJECT
    QML_ELEMENT
public:
    using Self = MdColorMgr;

    MdColorMgr(QObject* = nullptr);

    enum ColorSchemeEnum
    {
        Light,
        Dark
    };
    Q_ENUMS(ColorSchemeEnum)

    Q_PROPERTY(ColorSchemeEnum colorScheme READ colorScheme WRITE set_colorScheme NOTIFY
                   colorSchemeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE set_accentColor NOTIFY accentColorChanged)
    Q_PROPERTY(
        bool useSysColorSM READ useSysColorSM WRITE set_useSysColorSM NOTIFY useSysColorSMChanged)
    Q_PROPERTY(bool useSysAccentColor READ useSysAccentColor WRITE set_useSysAccentColor NOTIFY
                   useSysAccentColorChanged)

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
    ColorSchemeEnum colorScheme() const;
    ColorSchemeEnum sysColorScheme() const;

    QColor sysAccentColor() const;
    QColor accentColor() const;
    bool   useSysColorSM() const;
    bool   useSysAccentColor() const;

    Q_INVOKABLE QColor getOn(QColor) const;

public slots:
    void set_colorScheme(ColorSchemeEnum);
    void set_accentColor(QColor);
    void set_useSysColorSM(bool);
    void set_useSysAccentColor(bool);
    void gen_scheme();
    void refrehFromSystem();

signals:
    void colorSchemeChanged();
    void schemeChanged();
    void accentColorChanged();
    void useSysColorSMChanged();
    void useSysAccentColorChanged();

private:
    QColor                                  m_accent_color;
    ColorSchemeEnum                         m_color_scheme;
    qcm::MdScheme                           m_scheme;
    std::map<QColor, QColor, QColorCompare> m_on_map;
    bool                                    m_use_sys_color_scheme;
    bool                                    m_use_sys_accent_color;
};

} // namespace qml_material
