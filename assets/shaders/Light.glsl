uniform float uConCenter;
uniform float ucontrast;

vec3 contrast(vec3 v) {
    v.x = smoothstep(uConCenter - ucontrast, uConCenter + ucontrast, v.x);
    v.y = smoothstep(uConCenter - ucontrast, uConCenter + ucontrast, v.y);
    v.z = smoothstep(uConCenter - ucontrast, uConCenter + ucontrast, v.z);
    return v;
}

// I have no idea how this work
/*
float LightFallOut(float n, float m, float l, float mul) {
    return log(l / min(n, m)) * n * mul;
}
 */
vec3 LightFallOut(vec3 g, float m, float l, float mul) {
    vec3 b = clamp(g, 1e-4, m); // log(0)과 '밑이 1'인 경우를 피함
    return log(l) / log(b) * mul;
}

// ---------------- v2 헬퍼 ----------------
// 파장 → RGB 근사. t: 0 = 빨강(장파장) .. 1 = 보라(단파장)
vec3 spectrumRGB(float t) {
    vec3 x = (t - vec3(0.12, 0.48, 0.82)) * vec3(3.2, 3.4, 3.4);
    vec3 c = max(1.0 - x * x, 0.0);
    float v = (t - 1.0) * 5.0;
    c.r += 0.3 * max(1.0 - v * v, 0.0); // 보라 끝의 빨강 성분
    return c;
}

// 필름형 톤맵: 하이라이트가 하드 클립 대신 부드럽게 포화
vec3 tonemap(vec3 x) {
    return 1.0 - exp(-x);
}
