#include "Noise.glsl"
#include "Light.glsl"

in vec2 vUV;
out vec4 FragColor;

uniform float uTime;
uniform float uToffset; // RGB 채널 간 시간 위상차
uniform float uboost;
uniform float uradius; // 그라디언트가 0이 되는 반지름
uniform float ufadeRad; // 가장자리 안전 마스크 폭
uniform float uFall;

float NOISE_AMT = 0.6; // 영상 값: 노이즈가 그라디언트를 흔드는 양
const float GRAD_START = -0.07; // 영상 값: Map Range From Min

float channelNoise(float a, float r, float phase) {
    vec2 p = vec2(a * 5.0, r * 3.7);
    vec2 flow = vec2(-0.5 * ((uTime + phase) - (r * r)), 0.3 * uTime);
    return fbmWrap(p + flow, 25.0);
}

void main() {
    // 극좌표 (좌우 대칭)
    vec2 c = vUV * 2.0;

    float a = abs(atan(c.x, c.y) / 6.2831853);
    float s = sin(a * 6.2831853); // 위·아래 0, 좌·우 1
    float r = length(c) / max(s, 0.5); // 위·아래에서 r → 커짐 → 어두움

    vec3 ph = vec3(0.0, 1.0, 2.0) * uToffset;
    vec3 n = vec3(channelNoise(a, r, ph.r),
            channelNoise(a, r, ph.g),
            channelNoise(a, r, ph.b));

    // center gradient: 노이즈로 흔든 r → 중심이 밝은 그라디언트
    vec3 g = 1.0 - smoothstep(GRAD_START, uradius, r + n * NOISE_AMT);

    // 빛 퍼짐
    vec3 col = LightFallOut(g, 0.9, 0.9, 1.0);

    // 가장자리 안전 마스크
    float m = 1.0 - smoothstep(uradius, uradius + ufadeRad, r);
    col = contrast(col);
    col *= uboost * m;
    FragColor = vec4(col, max(col.r, max(col.g, col.b)));
}
