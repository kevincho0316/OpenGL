layout(location = 0) in vec2 aPos;

uniform vec4 uRect; // x, y, w, h (NDC)
uniform mat4 transform;

out vec2 vUV;

void main() {
    gl_Position = transform * vec4(aPos * uRect.zw + uRect.xy, 0.0, 1.0);
    vUV = aPos;
}
