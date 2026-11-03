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
