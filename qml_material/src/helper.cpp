#include "qml_material/helper.h"

#include "cpp/scheme/scheme.h"
#include "cpp/blend/blend.h"

#include "core/core.h"

#include "cpp/palettes/core.h"

using MdScheme = qcm::MdScheme;
namespace md   = material_color_utilities;

namespace
{

QRgb blend(QRgb a, QRgb b, double t) {
    double dt = 1.0 - t;
    return qRgba(qRed(a) * dt + qRed(b) * t,
                 qGreen(a) * dt + qGreen(b) * t,
                 qBlue(a) * dt + qBlue(b) * t,
                 qAlpha(a));
}
} // namespace

DEFINE_CONVERT(MdScheme, md::Scheme) {
    out.primary                = in.primary;
    out.on_primary             = in.on_primary;
    out.primary_container      = in.primary_container;
    out.on_primary_container   = in.on_primary_container;
    out.secondary              = in.secondary;
    out.on_secondary           = in.on_secondary;
    out.secondary_container    = in.secondary_container;
    out.on_secondary_container = in.on_secondary_container;
    out.tertiary               = in.tertiary;
    out.on_tertiary            = in.on_tertiary;
    out.tertiary_container     = in.tertiary_container;
    out.on_tertiary_container  = in.on_tertiary_container;
    out.error                  = in.error;
    out.on_error               = in.on_error;
    out.error_container        = in.error_container;
    out.on_error_container     = in.on_error_container;
    out.background             = in.background;
    out.on_background          = in.on_background;
    out.surface                = in.surface;
    out.on_surface             = in.on_surface;
    out.surface_variant        = in.surface_variant;
    out.on_surface_variant     = in.on_surface_variant;
    out.outline                = in.outline;
    out.outline_variant        = in.outline_variant;
    out.shadow                 = in.shadow;
    out.scrim                  = in.scrim;
    out.inverse_surface        = in.inverse_surface;
    out.inverse_on_surface     = in.inverse_on_surface;
    out.inverse_primary        = in.inverse_primary;
    out.surface_1              = blend(out.surface, out.primary, 0.05);
    out.surface_2              = blend(out.surface, out.primary, 0.08);
    out.surface_3              = blend(out.surface, out.primary, 0.11);
    out.surface_4              = blend(out.surface, out.primary, 0.12);
    out.surface_5              = blend(out.surface, out.primary, 0.14);
}

MdScheme qcm::MaterialLightColorScheme(QRgb rgb) {
    auto palette         = md::CorePalette::Of(rgb);
    auto scheme          = convert_from<MdScheme>(md::MaterialLightColorSchemeFromPalette(palette));
    scheme.background    = palette.neutral().get(98);
    scheme.on_background = palette.neutral().get(10);

    scheme.surface                   = palette.neutral().get(98);
    scheme.surface_dim               = palette.neutral().get(87);
    scheme.surface_bright            = palette.neutral().get(98);
    scheme.surface_container         = palette.neutral().get(94);
    scheme.surface_container_low     = palette.neutral().get(96);
    scheme.surface_container_lowest  = palette.neutral().get(100);
    scheme.surface_container_high    = palette.neutral().get(92);
    scheme.surface_container_highest = palette.neutral().get(90);

    return scheme;
}

MdScheme qcm::MaterialDarkColorScheme(QRgb rgb) {
    auto palette         = md::CorePalette::Of(rgb);
    auto scheme          = convert_from<MdScheme>(md::MaterialDarkColorSchemeFromPalette(palette));
    scheme.background    = palette.neutral().get(6);
    scheme.on_background = palette.neutral().get(90);

    scheme.surface                   = palette.neutral().get(6);
    scheme.surface_dim               = palette.neutral().get(6);
    scheme.surface_bright            = palette.neutral().get(24);
    scheme.surface_container         = palette.neutral().get(12);
    scheme.surface_container_low     = palette.neutral().get(10);
    scheme.surface_container_lowest  = palette.neutral().get(4);
    scheme.surface_container_high    = palette.neutral().get(17);
    scheme.surface_container_highest = palette.neutral().get(22);

    return scheme;
}

QRgb qcm::MaterialBlendHctHue(const QRgb design_color, const QRgb key_color, const double mount) {
    return md::BlendHctHue(design_color, key_color, mount);
}
