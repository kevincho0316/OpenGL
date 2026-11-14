// Bloom downsample: 13-tap filter. First pass also does the bright pass
// (soft-knee threshold), matching Blender Glare/Bloom threshold + smoothness.
#include "PostCommon.glsl"

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uSrc;
uniform vec2 uTexel;      // 1 / source size
uniform int uPrefilter;   // 1 on the first pass (HDR scene -> mip 0)
uniform float uThreshold; // 1.0
uniform float uKnee;      // 0.1

vec3 tap(float x, float y) { return texture(uSrc, vUV + uTexel * vec2(x, y)).rgb; }

void main() {
    vec3 a = tap(-2.0, 2.0), b = tap(0.0, 2.0), c = tap(2.0, 2.0);
    vec3 d = tap(-2.0, 0.0), e = tap(0.0, 0.0), f = tap(2.0, 0.0);
    vec3 g = tap(-2.0, -2.0), h = tap(0.0, -2.0), i = tap(2.0, -2.0);
    vec3 j = tap(-1.0, 1.0), k = tap(1.0, 1.0);
    vec3 l = tap(-1.0, -1.0), m = tap(1.0, -1.0);

    vec3 col = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 +
               (j + k + l + m) * 0.125;

    if (uPrefilter == 1) {
        col = clamp(col, 0.0, 1000.0);
        float br = max(col.r, max(col.g, col.b));
        float soft = clamp(br - uThreshold + uKnee, 0.0, 2.0 * uKnee);
        soft = soft * soft / (4.0 * uKnee + 1e-4);
        col *= max(soft, br - uThreshold) / max(br, 1e-4);
    }
    FragColor = vec4(col, 1.0);
}
