// Signed 3D Perlin gradient noise, output about [-1, 1].
// Classic "improved Perlin" (12 edge gradients, quintic fade) on top of a
// pcg3d integer hash. Not bit-identical to Blender's noise (not required).

uvec3 pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

float perlinGrad3(ivec3 cell, highp vec3 d) {
    uint h = (pcg3d(uvec3(cell)).x >> 16u) & 15u;
    float u = h < 8u ? d.x : d.y;
    float v = h < 4u ? d.y : ((h == 12u || h == 14u) ? d.x : d.z);
    return ((h & 1u) == 0u ? u : -u) + ((h & 2u) == 0u ? v : -v);
}

float snoise(highp vec3 p) {
    highp vec3 fl = floor(p);
    highp vec3 f = p - fl;
    ivec3 i = ivec3(fl);
    vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float n000 = perlinGrad3(i, f);
    float n100 = perlinGrad3(i + ivec3(1, 0, 0), f - vec3(1.0, 0.0, 0.0));
    float n010 = perlinGrad3(i + ivec3(0, 1, 0), f - vec3(0.0, 1.0, 0.0));
    float n110 = perlinGrad3(i + ivec3(1, 1, 0), f - vec3(1.0, 1.0, 0.0));
    float n001 = perlinGrad3(i + ivec3(0, 0, 1), f - vec3(0.0, 0.0, 1.0));
    float n101 = perlinGrad3(i + ivec3(1, 0, 1), f - vec3(1.0, 0.0, 1.0));
    float n011 = perlinGrad3(i + ivec3(0, 1, 1), f - vec3(0.0, 1.0, 1.0));
    float n111 = perlinGrad3(i + ivec3(1, 1, 1), f - vec3(1.0, 1.0, 1.0));

    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
               mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y), u.z);
}
