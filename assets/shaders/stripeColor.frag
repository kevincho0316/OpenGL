uniform vec4 uStripeColor;
uniform float uGap; // 줄무늬 한 칸 길이 (px)
uniform float uStripeSpeed;
uniform highp float uTime;

in vec2 vCoord;
out vec4 FragColor;

void main() {
    float x = vCoord.x - uTime * uStripeSpeed; // 시간에 따라 패턴이 이동
    if (int(floor(x / uGap)) % 2 == 0)
        FragColor = uStripeColor;
    else
        FragColor = vec4(0.0); // 투명
}
