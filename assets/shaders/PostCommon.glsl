precision highp float;
#ifdef GL_ES
precision highp sampler2D; // default lowp would crush HDR values
#endif

// 3x3 tent filter. radius is in UV units (texel size * filter radius).
vec3 tent9(sampler2D t, vec2 uv, vec2 r) {
    vec3 s = texture(t, uv).rgb * 4.0;
    s += (texture(t, uv + vec2(-r.x, 0.0)).rgb + texture(t, uv + vec2(r.x, 0.0)).rgb +
          texture(t, uv + vec2(0.0, -r.y)).rgb + texture(t, uv + vec2(0.0, r.y)).rgb) * 2.0;
    s += texture(t, uv + vec2(-r.x, -r.y)).rgb + texture(t, uv + vec2(r.x, -r.y)).rgb +
         texture(t, uv + vec2(-r.x, r.y)).rgb + texture(t, uv + vec2(r.x, r.y)).rgb;
    return s / 16.0;
}
