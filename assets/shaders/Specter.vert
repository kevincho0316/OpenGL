// "Butterfly Specter" particles: instanced camera-facing quads.
layout(location = 0) in vec2 aPos;      // -0.5 .. 0.5 quad corner
layout(location = 1) in vec4 iPosScale; // per instance: xyz = world pos, w = scale
layout(location = 2) in vec2 iLifeRnd;  // per instance: x = life 0..1, y = random 0..1

uniform mat4 view;
uniform mat4 projection;
uniform float uUvScale; // UV_SCALE: quad UV runs -uUvScale .. +uUvScale

out vec2 vUV;
flat out float vLife;
flat out float vRandom;

void main() {
    vec2 c = aPos * 2.0; // -1 .. 1
    // Billboard: expand in view space. scale = half extent (Blender plane is 2 m wide).
    vec3 center = (view * vec4(iPosScale.xyz, 1.0)).xyz;
    gl_Position = projection * vec4(center + vec3(c * iPosScale.w, 0.0), 1.0);
    vUV = c * uUvScale;
    vLife = iLifeRnd.x;
    vRandom = iLifeRnd.y;
}
