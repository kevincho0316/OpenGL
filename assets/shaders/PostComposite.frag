// Final pass: HDR scene + bloom -> colour boost -> tonemap -> sRGB.
#include "PostCommon.glsl"

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;     // bloom mip 0 (half resolution)
uniform vec2 uBloomTexel;     // 1 / bloom mip 0 size
uniform float uBloomRadius;   // tent radius in bloom texels
uniform float uBloomStrength; // already divided by the mip count
uniform float uSaturation;    // 1.3 ("Color Boost", luminance preserved)
uniform int uTonemap;         // 0 = clamp only, 1 = ACES

// ACES filmic fit (Narkowicz)
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 col = texture(uScene, vUV).rgb;
    col += tent9(uBloom, vUV, uBloomTexel * uBloomRadius) * uBloomStrength;

    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));
    col = max(vec3(luma) + (col - vec3(luma)) * uSaturation, 0.0);

    col = (uTonemap == 1) ? aces(col) : clamp(col, 0.0, 1.0);
    FragColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0); // manual sRGB gamma
}
