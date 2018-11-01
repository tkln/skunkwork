#version 410

#include "uniforms.glsl"
#include "hg_sdf.glsl"

out vec4 fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / uRes.xy;
    fragColor = vec4(uv + 0.5 * uMPos, 0.5 + 0.5 * sin(uTime), 1);
}
