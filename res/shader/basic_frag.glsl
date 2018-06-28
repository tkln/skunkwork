#version 410

#include "uniforms.glsl"
#include "hg_sdf.glsl"

mat3 camOrient(vec3 eye, vec3 target, vec3 up)
{
    vec3 n = normalize(target - eye);
    vec3 u = normalize(cross(up, n));
    vec3 v = cross(n, u);
    return mat3(u, v, n);
}

vec3 getViewRay(vec2 fragCoord, vec2 resolution, float fov)
{
    vec2 xy = fragCoord - resolution * 0.5;
    float z = resolution.y / tan(radians(fov * 0.5));
    return normalize(vec3(xy, z));
}


// Modified and cleaned up from the original

// Description : Array and textureless GLSL 2D/3D/4D simplex 
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20110822 (ijm)
//     License : MIT License
//
//Copyright (C) 2011 by Ashima Arts
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// https://github.com/ashima/webgl-noise
// https://github.com/stegu/webgl-noise

vec3 mod289(vec3 x) { return x - floor(x * 0.00346020761) * 289.0; }

vec4 mod289(vec4 x) { return x - floor(x * 0.00346020761) * 289.0; }

vec4 permute(vec4 x) { return mod289(((x * 34.0) + 1.0) * x); }

vec4 taylorInvSqrt(vec4 r) { return 1.792842914 - 0.8537347209 * r; }

float snoise(vec3 v)
{
    const vec2 C = vec2(0.166666667, 0.333333333);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);

    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;

    // Permutations
    i = mod289(i);
    vec4 p = permute(permute(permute(i.z + vec4(0.0, i1.z, i2.z, 1.0)) +
                             i.y + vec4(0.0, i1.y, i2.y, 1.0)) +
                     i.x + vec4(0.0, i1.x, i2.x, 1.0));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142; // 1.0/7.0
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_ );

    vec4 x = x_ * ns.x + ns.yyyy;
    vec4 y = y_ * ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4(x.xy, y.xy);
    vec4 b1 = vec4(x.zw, y.zw);

    vec4 s0 = floor(b0) * 2.0 + 1.0;
    vec4 s1 = floor(b1) * 2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    vec3 p0 = vec3(a0.xy, h.x);
    vec3 p1 = vec3(a0.zw, h.y);
    vec3 p2 = vec3(a1.xy, h.z);
    vec3 p3 = vec3(a1.zw, h.w);

    //Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
  }

//--------------------------- end of noise3D.glsl -----------------------------

const int FBM_OCTAVES = 5;

//----------------- https://www.shadertoy.com/view/4dS3Wd ---------------------

// By Morgan McGuire @morgan3d, http://graphicscodex.com
// Reuse permitted under the BSD license.
float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100);
	for (int i = 0; i < FBM_OCTAVES; ++i) {
		v += a * snoise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

//------------------------ End of shadertoy 4dS3Wd ----------------------------


struct Material {
    vec3 albedo;
    float roughness;
    float metalness;
    vec3 emissivity;
};

struct SceneResult {
    float dist;
    float materialIndex;
};

SceneResult opU(SceneResult r1, SceneResult r2) {
    return r2.dist < r1.dist ? r2 : r1;
}

SceneResult opI(SceneResult r1, SceneResult r2) {
    return r2.dist > r1.dist ? r2 : r1;
}

SceneResult scene(vec3 p)
{
    return opU(SceneResult(fSphere(p - vec3(0), 0.5), 0),
               SceneResult(min(min(min(fPlane(p, vec3(0,1,0), 4),
                                       fPlane(p, vec3(0,-1,0), 4)),
                                   min(fPlane(p, vec3(1,0,0), 4),
                                       fPlane(p, vec3(-1,0,0), 4))),
                               min(fPlane(p, vec3(0,0,1), 4),
                                   fPlane(p, vec3(0,0,-1), 4))),
                           1));
}

Material evalMaterial(vec3 p, float matI)
{
    Material mat = Material(vec3(0), 1, 0, vec3(2));
    if (matI > 0) {
        mat.emissivity = vec3(1);
    }
    return mat;
}

// Parameters
#define MAX_MARCHING_STEPS  256
#define MIN_DIST            0
#define MAX_DIST            100
#define EPSILON             0.0001

out vec4 fragColor;

vec3 lightPos = vec3(0);
vec3 lightInt = vec3(1, 0.5, 0.2);

struct HitInfo {
    vec3 color;
    vec3 normal;
    Material material;
};

vec3 getN(vec3 p)
{
    vec3 e = vec3(EPSILON, 0, 0);
    vec3 n = vec3(scene(vec3(p + e.xyy)).dist - scene(vec3(p - e.xyy)).dist,
                  scene(vec3(p + e.yxy)).dist - scene(vec3(p - e.yxy)).dist,
                  scene(vec3(p + e.yyx)).dist - scene(vec3(p - e.yyx)).dist);
    return normalize(n);
}

SceneResult castRay(vec3 rd, vec3 ro)
{
    float depth = MIN_DIST;
    SceneResult result;
    for (int i = 0; i < MAX_MARCHING_STEPS; ++i) {
        result = scene(ro + depth * rd);
        if (result.dist < depth * EPSILON) break;

        depth += result.dist;

        if (depth > MAX_DIST) break;
    }
    result.dist = depth;
    return result;
}

HitInfo evalHit(vec3 p, vec3 rd, float matI)
{
    HitInfo info;
    // Retrieve material for hit
    info.material = evalMaterial(p, matI);
    info.color = info.material.emissivity;
    return info;
}

vec3 evalFlame(vec3 camPos, vec3 rayDir, float depth) {

    float opacity = 0;
    float strength = 1 + sin(uTime);
    for (int i = 0; i < 6; ++i){
        vec3 ray = camPos + depth * rayDir;
        SceneResult result = scene(ray);
        if (result.dist > 0) break;

        float noise = fbm(ray * 2 - vec3(0, uTime * 2, 0) );
        opacity += pow((0.5 - length(ray)) * pow(noise * strength + strength - ray.y * 0.25, strength), (3 - strength) * (ray.y + length(ray) * 1.5));
        depth += noise * 0.1;

        if (depth > MAX_DIST) break;
    }
    return vec3(2 * opacity, 2 * opacity*opacity, opacity*opacity*opacity*opacity);
}

vec3 evalColor(vec3 camPos, vec3 rayDir, float depth) {
    vec3 hitPos = camPos + rayDir * depth;
    vec3 n = getN(hitPos);
    vec3 toLight = lightPos - hitPos;
    float lightDist = length(toLight);
    toLight /= lightDist;
    float nol = max(dot(n, toLight), 0);
    return (10 * sin(uTime)) * lightInt * nol / (lightDist * lightDist);
}

void main()
{
    //vec3 camPos = vec3(sin(uTime) * 2, 1, cos(uTime) * 2);
    vec3 camPos = vec3(-3, 3, -2);
    vec3 camTarget = vec3(0, 0, 0);
    vec3 rayDir = getViewRay(gl_FragCoord.xy, uRes, 65);
    rayDir = camOrient(camPos, camTarget, vec3(0, 1, 0)) * rayDir;

    // Cast a ray into scene
    SceneResult result = castRay(rayDir, camPos);

    // Check if it missed
    if (result.dist > MAX_DIST - EPSILON) {
        fragColor = vec4(0);
        return;
    }

    vec3 color = result.materialIndex < 1 ? evalFlame(camPos, rayDir, result.dist + 0.1) : evalColor(camPos, rayDir, result.dist);
    fragColor = vec4(color, 1);
}
