#version 410

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform mat4 uModelToWorld;
uniform mat3 uNormalToWorld;
uniform mat4 uWorldToClip;

out vec3 worldPos;
out vec3 worldNormal;

void main()
{
   vec4 wPos = uModelToWorld * vec4(pos, 1);
   worldPos = wPos.xyz;
   worldNormal = uNormalToWorld * normal;
   gl_Position = uWorldToClip * wPos;
}
