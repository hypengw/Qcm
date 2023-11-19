#version 440

#extension GL_GOOGLE_include_directive : enable

#include "sdf.glsl"

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4  qt_Matrix;
    float qt_Opacity;
    // tl,tr,bl,br
    vec4  radius;
    float size;
};
layout(binding = 1) uniform sampler2D source;

void main() {
    vec2 p = qt_TexCoord0 - vec2(0.5, 0.5);

    float sdf = sdf_rounded_rectangle(p, vec2(0.5, 0.5), radius / size);

    fragColor = sdf_render(sdf, vec4(0, 0, 0, 0), texture(source, qt_TexCoord0));
}