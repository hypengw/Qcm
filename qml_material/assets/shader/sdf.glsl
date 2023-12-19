// Signed distance function

// according to https://iquilezles.org/articles/distfunctions2d/

float sdf_circle(vec2 p, float r) { return length(p) - r; }

float sdf_rectangle(in vec2 p, in vec2 rect) {
    vec2 d = abs(p) - rect;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// tr,br,bl,tl
float sdf_rounded_rectangle(in vec2 p, in vec2 rect, in vec4 r) {
    r.xy   = (p.x > 0.0) ? r.xy : r.zw;
    r.x    = (p.y > 0.0) ? r.x : r.y;
    vec2 q = abs(p) - rect + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

const float sdf_default_smoothing = 0.625;

vec4 sdf_render(in float sdf, in vec4 sourceColor, in vec4 sdfColor, in float alpha, in float smoothing, in float offset)
{
    // bigger when zoom out
    float g = smoothing * fwidth(sdf);
    return mix(sourceColor, sdfColor, alpha * (1.0 - clamp((1.0 / g) * sdf - offset, 0.0, 1.0)));
}

vec4 sdf_render(in float sdf, in vec4 sourceColor, in vec4 sdfColor, in float alpha, in float smoothing)
{
    return sdf_render(sdf, sourceColor, sdfColor, alpha, smoothing, 0);
}

vec4 sdf_render(in float sdf, in vec4 sourceColor, in vec4 sdfColor, in float alpha)
{
    return sdf_render(sdf, sourceColor, sdfColor, alpha, sdf_default_smoothing);
}

vec4 sdf_render(in float sdf, in vec4 sourceColor, in vec4 sdfColor)
{
    return sdf_render(sdf, sourceColor, sdfColor, 1.0);
}