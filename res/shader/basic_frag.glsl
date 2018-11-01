#version 410

#include "uniforms.glsl"
#include "hg_sdf.glsl"

uniform vec3 dColor;

out vec4 fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / uRes.xy;
    vec3 color = dColor + vec3(0.5 * uMPos + uv, 0.5 * sin(uTime) + 0.5);
    fragColor = vec4(color, 1);
}
