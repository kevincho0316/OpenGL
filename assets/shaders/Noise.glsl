uvec2 pcg2d(uvec2 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v ^= v >> 16u;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v ^= v >> 16u;
    return v;
}

vec2 grad(highp vec2 i) {
    highp float a = float(pcg2d(uvec2(ivec2(i))).x) * (6.2831853 / 4294967296.0);
    return vec2(cos(a), sin(a));
}

// x 방향이 period 칸마다 반복되는 perlin (각도용)
float perlinWrapX(highp vec2 p, float period) {
    highp vec2 i = floor(p);
    highp vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float x0 = mod(i.x, period); // 마지막 칸의 오른쪽 = 첫 칸의 왼쪽
    float x1 = mod(i.x + 1.0, period);

    float a = dot(grad(vec2(x0, i.y)), f);
    float b = dot(grad(vec2(x1, i.y)), f - vec2(1, 0));
    float c = dot(grad(vec2(x0, i.y + 1.0)), f - vec2(0, 1));
    float d = dot(grad(vec2(x1, i.y + 1.0)), f - vec2(1, 1));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float perlin(highp vec2 p) {
    highp vec2 i = floor(p);
    highp vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    float a = dot(grad(i), f);
    float b = dot(grad(i + vec2(1, 0)), f - vec2(1, 0));
    float c = dot(grad(i + vec2(0, 1)), f - vec2(0, 1));
    float d = dot(grad(i + vec2(1, 1)), f - vec2(1, 1));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbmWrap(highp vec2 p, float period, float gain) {
    float s = 0.0, a = 0.5, norm = 0.0;
    for (int k = 0; k < 5; k++) {
        s += a * perlinWrapX(p, period);
        norm += a;
        p *= 2.0;
        period *= 2.0; // 좌표를 2배로 늘렸으니 주기도 같이 2배로
        a *= gain;
    }
    return s / norm; // gain 값과 상관없이 범위를 일정하게
}

float fbmWrap(highp vec2 p, float period) { // 기본값 대신 오버로드 사용
    return fbmWrap(p, period, 0.5);
}

// ---------------- v2 헬퍼 ----------------
#ifndef FBM_OCT
#define FBM_OCT 5
#endif

// 감싸지 않는 일반 fbm (범위 대략 -0.5..0.5)
float fbm(highp vec2 p) {
    float s = 0.0, a = 0.5, norm = 0.0;
    for (int k = 0; k < FBM_OCT; k++) {
        s += a * perlin(p);
        norm += a;
        p = p * 2.0 + vec2(17.13, 4.71); // 옥타브마다 격자 정렬 깨기
        a *= 0.5;
    }
    return s / norm;
}

// 2단 domain warp: p → w1(p) → w2(p + w1). 반환값 = 최종 변위 벡터
vec2 warp2(highp vec2 p, float t) {
    vec2 w1 = vec2(fbm(p + t * vec2(0.11, 0.07)),
                   fbm(p + vec2(5.2, 1.3) - t * vec2(0.05, 0.09)));
    highp vec2 q = p + 2.0 * w1;
    return vec2(fbm(q + vec2(1.7, 9.2)),
                fbm(q + vec2(8.3, 2.8)));
}

