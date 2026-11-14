// "Butterfly Specter" fragment shader: direct translation of the Blender node
// graph. Renders premultiplied HDR (blend GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
precision highp float; // rnd * 33.33 * freq needs more than mediump on GLES

#include "Noise3D.glsl"

in vec2 vUV;
flat in float vLife;
flat in float vRandom;
out vec4 FragColor;

uniform float uEmissionStrength;  // 15.0
uniform float uChannelOffsetStep; // 1.0 (offsets 0, 1, 2)
uniform float uNoiseMul;          // 0.17
uniform vec3 uNoiseFreq;          // (5, 3.7, 10)
uniform float uTimeMul;           // 4.0
uniform int uDebugMode;           // 0 = final, 1 = one specter() channel grey, 2 = polar
uniform int uDebugChannel;        // channel shown in debug mode 1 (0, 1, 2)

const float PI = 3.14159265359;
const float MAX_OUT = 1000.0; // keeps bloom from blowing up
// Beyond this radius the premultiplied output is < 2e-4 (invisible after
// tonemapping), so skip the 18 noise evaluations. Purely an optimisation.
const float CULL_RADIUS = 0.8;

// ---------- Blender math helpers ----------
// Gradient Texture, type RADIAL
float blenderRadial(vec2 p) { return atan(p.y, p.x) / (2.0 * PI) + 0.5; }

// Math node: PINGPONG
float pingpong(float a, float s) {
    return (s != 0.0) ? abs(fract((a - s) / (s * 2.0)) * s * 2.0 - s) : 0.0;
}

// Math node: LOGARITHM, log of a in base b, 0 when undefined (Blender safe log)
float safeLog(float a, float b) {
    return (a > 0.0 && b > 0.0 && b != 1.0) ? log(a) / log(b) : 0.0;
}

float smootherstep01(float e0, float e1, float x) {
    x = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

// Map Range (FLOAT, SMOOTHSTEP): Blender inverts when fromMin > fromMax
float mapRangeSmooth(float v, float fMin, float fMax, float tMin, float tMax) {
    float f = (fMin > fMax) ? 1.0 - smoothstep(fMax, fMin, v) : smoothstep(fMin, fMax, v);
    return tMin + f * (tMax - tMin);
}
// Map Range (FLOAT, SMOOTHERSTEP)
float mapRangeSmoother(float v, float fMin, float fMax, float tMin, float tMax) {
    float f = (fMin > fMax) ? 1.0 - smootherstep01(fMax, fMin, v) : smootherstep01(fMin, fMax, v);
    return tMin + f * (tMax - tMin);
}

// ---------- Noise Texture: 3D, fBM, Normalize OFF ----------
float blenderFbm(vec3 p, float detail, float roughness, float lacunarity) {
    float sum = 0.0, amp = 1.0, freq = 1.0;
    int octaves = int(floor(detail));
    for (int i = 0; i <= octaves; i++) { // detail 2 -> 3 octaves
        sum += snoise(p * freq) * amp;
        amp *= roughness;
        freq *= lacunarity;
    }
    float rmd = detail - floor(detail); // fractional octave (0 for detail = 2)
    if (rmd != 0.0)
        sum += rmd * snoise(p * freq) * amp;
    return sum; // Normalize OFF -> raw signed sum
}

// Scale 1.0, Detail 2.0, Roughness 0.5, Lacunarity 2.0, Distortion 0.25
float noiseTexture(vec3 p) {
    p *= 1.0; // Scale
    // Distortion: offset p by noise sampled at fixed random offsets
    p += vec3(snoise(p + vec3(13.5, 41.2, 7.1)),
              snoise(p + vec3(-23.4, 5.7, 88.8)),
              snoise(p + vec3(61.3, -17.9, 30.2))) * 0.25;
    return blenderFbm(p, 2.0, 0.5, 2.0);
}

// ---------- "Lg Light Falloff" node group ----------
// ASSUMPTION: formula reverse-engineered from a video (value = Log, base = m).
// It behaves like an inverse-square falloff around g -> 1, which looks right,
// so the safeLog arguments were NOT swapped. If the core looks wrong, try
// safeLog(m, logV) instead.
float lgLightFalloff(float g, float minV, float logV, float mult) {
    float m = clamp(min(minV, g), 0.0, 1.0); // Minimum, clamp ON
    float l = safeLog(logV, m);              // Logarithm: value = Log, base = m
    return l * g * mult;
}

// ---------- polar uv ----------
// ASSUMPTION: X = angle, Y = radius (order not visible in the video; swap here)
vec3 polarUV(vec2 uv) {
    float angle = pingpong(blenderRadial(uv), 0.5); // mirrored angle -> wing symmetry
    return vec3(angle, length(uv), 0.0);
}

// ---------- "specter" node group: one colour channel ----------
// offset: 0, 1, 2 for R, G, B
float specter(vec2 uv, float life, float rnd, float offset) {
    vec3 polar = polarUV(uv);

    // time offset (driven by particle life; offset shifts each channel in time)
    float t = life * uTimeMul + offset * 0.1;
    vec3 q = polar + vec3(t) * vec3(0.1, -0.1, -0.05);

    // random offset per particle
    q = (q + vec3(rnd * 33.33)) * uNoiseFreq;

    float n = noiseTexture(q) * uNoiseMul;

    // center gradient offset
    float g = mapRangeSmooth(n + length(uv), -0.07, 1.5, 1.0, 0.0);

    // life fade
    g += mapRangeSmoother(life, 0.75, 1.0, 0.0, -0.12);

    return lgLightFalloff(g, 0.999, 0.999, 1.0);
}

float sane(float v) { return isnan(v) || isinf(v) ? 0.0 : v; }

void main() {
    if (uDebugMode == 2) { // polar as colour: R = angle (0..0.5 -> 0..1), G = radius
        vec3 p = polarUV(vUV);
        FragColor = vec4(p.x * 2.0, p.y, 0.0, 1.0);
        return;
    }
    if (uDebugMode == 1) { // one specter() channel as greyscale (Blender Viewer)
        float v = sane(specter(vUV, vLife, vRandom, float(uDebugChannel) * uChannelOffsetStep));
        FragColor = vec4(vec3(clamp(v, 0.0, 1.0)), 1.0);
        return;
    }

    if (length(vUV) > CULL_RADIUS)
        discard;

    vec3 col = vec3(
        sane(specter(vUV, vLife, vRandom, 0.0 * uChannelOffsetStep)),
        sane(specter(vUV, vLife, vRandom, 1.0 * uChannelOffsetStep)),
        sane(specter(vUV, vLife, vRandom, 2.0 * uChannelOffsetStep)));

    // Mix Shader(Transparent, Emission) with factor = clamp(hidden node)
    // ASSUMPTION: the hidden node is the max of the RGB channels
    float a = clamp(max(col.r, max(col.g, col.b)), 0.0, 1.0);
    vec3 emission = col * uEmissionStrength;

    FragColor = vec4(clamp(emission * a, 0.0, MAX_OUT), a); // premultiplied
}
