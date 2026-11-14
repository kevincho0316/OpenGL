// Bloom upsample: 3x3 tent, blended additively (GL_ONE, GL_ONE) onto the next
// larger mip.
#include "PostCommon.glsl"

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uSrc;
uniform vec2 uTexel;   // 1 / source size
uniform float uRadius; // filter radius in source texels

void main() { FragColor = vec4(tent9(uSrc, vUV, uTexel * uRadius), 1.0); }
