precision highp float;

in vec2 vCoord;
out vec4 fragColor;

void main() {
    vec2 coord = vCoord;
    vec2 d = fwidth(coord); // 화면 1픽셀당 좌표 변화량
    vec2 g = abs(fract(coord - 0.5) - 0.5) / d;
    float line = min(g.x, g.y); // 선까지 거리(픽셀 단위)
    float a = 1.0 - min(line, 1.0); // 선 근처만 1
    fragColor = vec4(vec3(0.8), a);
}
