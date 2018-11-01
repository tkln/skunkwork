#version 410

#include "uniforms.glsl"
#include "hg_sdf.glsl"

out vec4 fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / uRes.xy;
    fragColor = vec4(0.5 * uMPos + uv, 0.5 * sin(uTime) + 0.5, 1);
}
