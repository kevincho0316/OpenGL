in vec2 vPx;
out vec4 FragColor;

uniform vec2 uSize;
uniform float uCorner;      // 모서리 반지름 (픽셀)
uniform float uBorder;      // 테두리 두께 (픽셀)
uniform vec4 uColor;        // 채움 색
uniform vec4 uBorderColor;  // 테두리 색

// 둥근 사각형 SDF: 안쪽 < 0
float roundRect(vec2 p, vec2 halfSize, float r) {
    vec2 q = abs(p) - halfSize + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    float d = roundRect(vPx, uSize * 0.5, uCorner);
    float shape = 1.0 - smoothstep(-1.0, 0.0, d);          // 1px 안티앨리어싱
    float border = smoothstep(-uBorder - 1.0, -uBorder, d); // 테두리 쪽 = 1
    vec4 c = mix(uColor, uBorderColor, border);
    if (shape <= 0.0)
        discard;
    FragColor = vec4(c.rgb, c.a * shape);
}
