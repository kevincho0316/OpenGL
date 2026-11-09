layout(location = 0) in vec2 aPos;

uniform vec2 uScreen; // 화면 크기 (픽셀)
uniform vec2 uAnchor; // 화살표 꼬리 위치 (픽셀, 왼쪽 위 원점)
uniform vec2 uDir; // 방향 벡터 (픽셀 좌표계, y 아래로 증가)
uniform float uSize; // 화살표 전체 길이 (픽셀)

void main() {
    vec2 d = normalize(uDir);
    vec2 perp = vec2(-d.y, d.x); // d를 90도 돌린 벡터
    vec2 px = uAnchor + (d * aPos.x + perp * aPos.y) * uSize;

    // 픽셀 → 클립 좌표 (y 뒤집기)
    gl_Position = vec4(px.x / uScreen.x * 2.0 - 1.0,
            1.0 - px.y / uScreen.y * 2.0,
            0.0, 1.0);
}
