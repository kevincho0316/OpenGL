// ButterflyFg v2: 크리스탈 굴절광 (프리즘 부채꼴 광선)
// - 작은 흰 코어 + 코어에서 뻗는 직선 광선 (각도에만 의존하는 결)
// - 부채꼴 로브: 날개 2개 + 아래 꼬리 + 위 스파이크 (좌우 대칭)
// - 분산: 파장마다 각도 스케일 다름 → 부채꼴 가장자리 무지개
//         파장마다 반지름 스케일 다름 → 빨강이 더 멀리 → 따뜻한 끝
#ifdef GL_ES
#define SPECTRAL_SAMPLES 8
#else
#define SPECTRAL_SAMPLES 16
#endif

#include "Noise.glsl"
#include "Light.glsl"

in vec2 vUV;
out vec4 FragColor;

uniform float uTime;
uniform float uboost;    // 최종 세기
uniform float uradius;   // 광선 길이 스케일
uniform float ufadeRad;  // 가장자리 안전 마스크 폭
uniform float uFall;     // 코어 주변 감쇠 (클수록 광선이 짧고 코어에 몰림)
uniform float uDisp;     // 각도 분산: 부채꼴 가장자리 무지개 폭
uniform float uDispR;    // 반지름 분산: 빨강이 더 멀리 (따뜻한 끝)
uniform float uExposure; // 톤맵 전 노출
uniform float uStreak;   // 광선 결 세기 (0 = 매끈한 부채꼴, 1 = 가는 줄)
uniform float uWingAng;  // 날개 방향 (위에서부터 라디안, 1.57 = 수평)
uniform float uWingW;    // 날개 부채꼴 폭 (라디안)

const float PI = 3.14159265;

float lobe(float phi, float ctr, float w) {
    float x = (phi - ctr) / w;
    return exp(-x * x);
}

// 각도 phi, 반지름 r 에서의 광선 세기 (한 파장)
float beams(float phi, float r, float wingAng) {
    float L = uradius;
    float wing  = lobe(phi, wingAng, uWingW) * exp(-r / (0.55 * L));
    float wing2 = lobe(phi, wingAng + 0.6, uWingW * 0.6) * 0.5 * exp(-r / (0.35 * L)); // 아래 작은 날개
    float tail  = lobe(phi, PI, 0.10) * 0.7 * exp(-r / (0.45 * L));
    float spike = lobe(phi, 0.0, 0.06) * 0.5 * exp(-r / (0.25 * L));
    return (wing + wing2 + tail + spike) / (1.0 + uFall * r);
}

void main() {
    highp vec2 c = vUV * 2.0;             // -1..1
    float r = length(c);
    float phi = abs(atan(c.x, c.y));      // 0 = 위, PI = 아래 (좌우 대칭)

    // 날개 각도가 천천히 숨쉬듯 움직임
    float wingAng = uWingAng + 0.08 * perlin(vec2(uTime * 0.2, 3.1));

    // 광선 결: 각도에만 의존 → 코어에서 직선으로 뻗는 줄
    float sN = 0.6 * perlin(vec2(phi * 38.0, uTime * 0.3))
             + 0.4 * perlin(vec2(phi * 95.0, uTime * 0.5 + 7.0));
    float str = clamp(0.5 + 1.4 * sN, 0.0, 1.0);
    // 코어 근처는 결이 흐려짐 (줄이 한 점에서 모이며 하얗게)
    float streak = mix(1.0, 1.6 * str * str, uStreak * smoothstep(0.0, 0.15, r));

    // 코어 + 옅은 헤이즈 (흰색, 분산 거의 없음)
    float core = 3.0 * exp(-r * r / 0.0025) + 0.012 / (r + 0.012);

    vec3 col = vec3(0.0);
    vec3 wsum = vec3(0.0);
    for (int i = 0; i < SPECTRAL_SAMPLES; i++) {
        float t = (float(i) + 0.5) / float(SPECTRAL_SAMPLES); // 0 = 빨강 .. 1 = 보라
        float sgn = 2.0 * t - 1.0;
        float phiL = phi * (1.0 + uDisp * sgn);  // 각도 분산
        float rL = r * (1.0 + uDispR * sgn);     // 반지름 분산 (빨강 rL 작음 → 더 멀리)

        vec3 w = spectrumRGB(t);
        col += w * beams(phiL, rL, wingAng);
        wsum += w;
    }
    col = col / max(wsum, vec3(1e-4)) * streak + core;

    col = tonemap(col * uExposure);

    float m = 1.0 - smoothstep(1.0 - ufadeRad, 1.0, r); // 쿼드 가장자리에서 0
    col *= uboost * m;
    FragColor = vec4(col, max(col.r, max(col.g, col.b)));
}
