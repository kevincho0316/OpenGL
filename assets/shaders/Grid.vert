layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 uViewProj;
uniform vec3 uCam;
uniform float uScale;
uniform int uPlane; // 0 = XZ, 1 = XY, 2 = YZ

out vec2 vCoord;

void main() {
    vec2 p = aPos.xy * uScale;
    vec3 world;
    if (uPlane == 0) {
        world = vec3(p.x + uCam.x, 0.0, p.y + uCam.z);
        vCoord = world.xz;
    } else if (uPlane == 1) {
        world = vec3(p.x + uCam.x, p.y + uCam.y, 0.0);
        vCoord = world.xy;
    } else {
        world = vec3(0.0, p.x + uCam.y, p.y + uCam.z);
        vCoord = world.yz;
    }
    gl_Position = projection * view * vec4(world, 1.0);
    gl_Position.z += 0.0005 * gl_Position.w; // 깊이만 살짝 뒤로
}
