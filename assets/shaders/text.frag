in vec2 vUV;

uniform sampler2D uTex;
uniform vec4 uColor;
out vec4 outColor;

void main() {
    float a = texture(uTex, vUV).r; // 아틀라스는 1채널이라 .r
    a = step(0.5, a); // x < 0.5 → 0.0, x >= 0.5 → 1.0
    outColor = vec4(uColor.rgb, uColor.a * a);
}
