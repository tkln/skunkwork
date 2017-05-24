#version 410

#include "hg_sdf.glsl"
#include "uniforms.glsl"

out vec4 fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / uRes.xy;
    fragColor = vec4(uv + 0.5 * uMPos, 0.5 + 0.5 * sin(uGT), 1);
}
