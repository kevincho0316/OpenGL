// 화면 픽셀 좌표(왼쪽 위 원점, y 아래로 증가)의 사각형
layout(location = 0) in vec2 aPos; // -0.5 ~ 0.5 쿼드
out vec2 vPx;                      // 버튼 중심 기준 픽셀 좌표

uniform vec2 uScreen; // 화면 크기 (픽셀)
uniform vec2 uPos;    // 버튼 왼쪽 위 (픽셀)
uniform vec2 uSize;   // 버튼 크기 (픽셀)

void main() {
    vPx = aPos * uSize;
    vec2 px = uPos + (aPos + 0.5) * uSize;
    gl_Position = vec4(px.x / uScreen.x * 2.0 - 1.0,
            1.0 - px.y / uScreen.y * 2.0,
            0.0, 1.0);
}
