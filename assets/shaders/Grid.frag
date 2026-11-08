in vec3 vWorldPos; // 버텍스 셰이더에서 넘겨받은 월드 좌표
out vec4 fragColor;

void main() {
    vec2 coord = vWorldPos.xz; // 1칸 = 1.0 단위
    vec2 d = fwidth(coord); // 화면 1픽셀당 좌표 변화량
    vec2 g = abs(fract(coord - 0.5) - 0.5) / d;
    float line = min(g.x, g.y); // 선까지 거리(픽셀 단위)
    float a = 1.0 - min(line, 1.0); // 선 근처만 1
    fragColor = vec4(vec3(0.8), a);
}
