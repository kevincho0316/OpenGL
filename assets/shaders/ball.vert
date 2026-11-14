// 구 임포스터: 카메라를 향한 사각형에 구를 그림 (perspective / ortho 모두 동작)
layout(location = 0) in vec2 aPos; // -0.5 ~ 0.5 쿼드
out vec2 vLocal;                   // -1 ~ 1 (구 안쪽 판별 + 노멀)

uniform mat4 view;
uniform mat4 projection;
uniform mat4 uModel; // Transform3D::matrix(), scale = 반지름

void main() {
    vec3 center = (view * uModel * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    float radius = length(uModel[0].xyz);
    vLocal = aPos * 2.0;
    vec3 p = center + vec3(vLocal * radius, 0.0); // 뷰 공간에서 펼침
    gl_Position = projection * vec4(p, 1.0);
}
