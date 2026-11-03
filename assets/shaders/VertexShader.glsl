layout(location = 0) in vec2 aPos;
uniform vec4 uRect; // x, y, w, h (NDC)
out vec2 vUV;

void main() {
    gl_Position = vec4(aPos * uRect.zw + uRect.xy, 0.0, 1.0);
    vUV = aPos;
}
