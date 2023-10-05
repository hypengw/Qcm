#include "material_helper/helper.h"

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

template<>
struct To<MdScheme> {
    static auto from(const md::Scheme& in) {
        MdScheme o;
        o.primary                = in.primary;
        o.on_primary             = in.on_primary;
        o.primary_container      = in.primary_container;
        o.on_primary_container   = in.on_primary_container;
        o.secondary              = in.secondary;
        o.on_secondary           = in.on_secondary;
        o.secondary_container    = in.secondary_container;
        o.on_secondary_container = in.on_secondary_container;
        o.tertiary               = in.tertiary;
        o.on_tertiary            = in.on_tertiary;
        o.tertiary_container     = in.tertiary_container;
        o.on_tertiary_container  = in.on_tertiary_container;
        o.error                  = in.error;
        o.on_error               = in.on_error;
        o.error_container        = in.error_container;
        o.on_error_container     = in.on_error_container;
        o.background             = in.background;
        o.on_background          = in.on_background;
        o.surface                = in.surface;
        o.on_surface             = in.on_surface;
        o.surface_variant        = in.surface_variant;
        o.on_surface_variant     = in.on_surface_variant;
        o.outline                = in.outline;
        o.outline_variant        = in.outline_variant;
        o.shadow                 = in.shadow;
        o.scrim                  = in.scrim;
        o.inverse_surface        = in.inverse_surface;
        o.inverse_on_surface     = in.inverse_on_surface;
        o.inverse_primary        = in.inverse_primary;
        o.surface_1              = blend(o.surface, o.primary, 0.05);
        o.surface_2              = blend(o.surface, o.primary, 0.08);
        o.surface_3              = blend(o.surface, o.primary, 0.11);
        o.surface_4              = blend(o.surface, o.primary, 0.12);
        o.surface_5              = blend(o.surface, o.primary, 0.14);
        return o;
    }
};

MdScheme qcm::MaterialLightColorScheme(QRgb rgb) {
    auto palette             = md::CorePalette::Of(rgb);
    auto scheme              = To<MdScheme>::from(md::MaterialLightColorSchemeFromPalette(palette));
    scheme.background = palette.neutral().get(98);
    scheme.on_background = palette.neutral().get(10);

    scheme.surface           = palette.neutral().get(98);
    scheme.surface_dim       = palette.neutral().get(87);
    scheme.surface_bright    = palette.neutral().get(98);
    scheme.surface_container = palette.neutral().get(94);
    scheme.surface_container_low     = palette.neutral().get(96);
    scheme.surface_container_lowest  = palette.neutral().get(100);
    scheme.surface_container_high    = palette.neutral().get(92);
    scheme.surface_container_highest = palette.neutral().get(90);

    return scheme;
}

MdScheme qcm::MaterialDarkColorScheme(QRgb rgb) {
    auto palette             = md::CorePalette::Of(rgb);
    auto scheme              = To<MdScheme>::from(md::MaterialDarkColorSchemeFromPalette(palette));
    scheme.background = palette.neutral().get(6);
    scheme.on_background = palette.neutral().get(90);

    scheme.surface           = palette.neutral().get(6);
    scheme.surface_dim       = palette.neutral().get(6);
    scheme.surface_bright    = palette.neutral().get(24);
    scheme.surface_container = palette.neutral().get(12);
    scheme.surface_container_low     = palette.neutral().get(10);
    scheme.surface_container_lowest  = palette.neutral().get(4);
    scheme.surface_container_high    = palette.neutral().get(17);
    scheme.surface_container_highest = palette.neutral().get(22);

    return scheme;
}

QRgb qcm::MaterialBlendHctHue(const QRgb design_color, const QRgb key_color, const double mount) {
    return md::BlendHctHue(design_color, key_color, mount);
}
