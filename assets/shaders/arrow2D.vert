// 우선 옆으로 누은 화살표를 만들고
//d와 perp를 기저 벡터 삼아서 선형변환함

layout(location = 0) in vec2 aPos;
out vec2 vCoord;

uniform vec2 uScreen; // 화면 크기 (픽셀)
uniform vec2 uAnchor; // 화살표 꼬리 위치 (픽셀, 왼쪽 위 원점)
uniform vec2 uDir; // 방향 벡터 (픽셀 좌표계, y 아래로 증가)
uniform float uSize; // 화살표 전체 길이 (픽셀)
void main() {
    vec2 d = normalize(uDir);
    vec2 perp = vec2(-d.y, d.x);

    float headLen = 130.0; // 머리 길이 (px, 고정)
    float headW = 170.0; // 머리 폭 (px, 고정)
    float thick = 60.0; // 몸통 두께 (px, 고정)
    float shaftLen = max(uSize - headLen, 0.0); // 가변: 몸통 길이

    vec2 local;
    if (gl_VertexID < 6) { // 앞 6개 정점 = 몸통
        float t = (aPos.x + 0.5) / 0.7; // 0~1로 정규화
        local = vec2(t * shaftLen, sign(aPos.y) * thick * 0.5);
    } else { // 뒤 3개 정점 = 머리
        float t = (aPos.x - 0.2) / 0.4; // 0 또는 1
        local = vec2(shaftLen + t * headLen, sign(aPos.y) * headW * 0.5);
    }

    vec2 px = uAnchor + d * local.x + perp * local.y;
    vCoord = local;
    gl_Position = vec4(px.x / uScreen.x * 2.0 - 1.0,
            1.0 - px.y / uScreen.y * 2.0,
            0.0, 1.0);
}
