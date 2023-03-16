#pragma once

#include <QtGui/qcolor.h>
#include <QObject>

namespace qcm
{
struct MdScheme {
    QRgb primary;
    QRgb on_primary;
    QRgb primary_container;
    QRgb on_primary_container;
    QRgb secondary;
    QRgb on_secondary;
    QRgb secondary_container;
    QRgb on_secondary_container;
    QRgb tertiary;
    QRgb on_tertiary;
    QRgb tertiary_container;
    QRgb on_tertiary_container;
    QRgb error;
    QRgb on_error;
    QRgb error_container;
    QRgb on_error_container;
    QRgb background;
    QRgb on_background;
    QRgb surface;
    QRgb on_surface;
    QRgb surface_variant;
    QRgb on_surface_variant;
    QRgb outline;
    QRgb outline_variant;
    QRgb shadow;
    QRgb scrim;
    QRgb inverse_surface;
    QRgb inverse_on_surface;
    QRgb inverse_primary;

    // surface
    QRgb surface_1;
    QRgb surface_2;
    QRgb surface_3;
    QRgb surface_4;
    QRgb surface_5;
};

MdScheme MaterialLightColorScheme(QRgb);
MdScheme MaterialDarkColorScheme(QRgb);

QRgb MaterialBlendHctHue(const QRgb design_color, const QRgb key_color, const double mount);
} // namespace qcm
