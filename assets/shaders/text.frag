in vec2 vUV;

uniform sampler2D uTex;
uniform vec4 uColor;
out vec4 outColor;

void main() {
    float a = texture(uTex, vUV).r; // 아틀라스는 1채널이라 .r
    outColor = vec4(uColor.rgb, uColor.a * a);
}
