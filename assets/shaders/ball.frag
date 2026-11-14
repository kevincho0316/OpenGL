in vec2 vLocal;
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    float r2 = dot(vLocal, vLocal);
    if (r2 > 1.0)
        discard;
    vec3 n = vec3(vLocal, sqrt(1.0 - r2));   // 뷰 공간 노멀
    vec3 l = normalize(vec3(0.4, 0.6, 0.7)); // 뷰 공간 조명 방향
    float diff = max(dot(n, l), 0.0);
    float spec = pow(max(dot(reflect(-l, n), vec3(0.0, 0.0, 1.0)), 0.0), 32.0);
    vec3 c = uColor.rgb; //* (0.25 + 0.75 * diff) + vec3(spec * 0.5);
    FragColor = vec4(c, uColor.a);
}
