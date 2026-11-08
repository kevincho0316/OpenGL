layout(location = 0) in vec3 aPos;

uniform mat4 uViewProj;
uniform vec2 uCamXZ;
uniform float uScale;

out vec3 vWorldPos;

void main() {
    vec3 world = vec3(aPos.x * uScale + uCamXZ.x,
            0.0,
            aPos.z * uScale + uCamXZ.y);
    vWorldPos = world;
    gl_Position = uViewProj * vec4(world, 1.0);
}
