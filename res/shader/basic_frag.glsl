#version 410

#include "uniforms.glsl"
#include "hg_sdf.glsl"
#include "noise.glsl"

// Shortcuts
uniform float dTmp;
uniform vec3 dColor;
uniform vec3 dDiffuse;
uniform vec3 dSpecular;
uniform float dSpecularPow;
uniform vec3 dPos;
uniform vec3 dRot;
uniform vec3 dSize;

// Camera
uniform vec3 dCPos;
uniform vec3 dCRot;

out vec4 fragColor;

const float MAX_DIST = 500;

struct Material {
    vec3 diffuse;
    vec3 specular;
    float specularPow;
    vec3 emission;
};

const vec3 LIGHT_POS = vec3(0, 0, 5);
const vec3 LIGHT_AINT = vec3(0, 0.3, 0);
const vec3 LIGHT_DINT = vec3(1.5);
const vec3 LIGHT_SINT= vec3(1.5);

// sRGB, linear space conversions
float stol(float x) { return (x <= 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4)); }
vec3 stol(vec3 c) { return vec3(stol(c.x), stol(c.y), stol(c.z)); }
float ltos(float x) { return (x <= 0.0031308 ? x * 12.92 : 1.055 * pow(x, 0.4166667) - 0.055); }
vec3 ltos(vec3 c) { return vec3(ltos(c.x), ltos(c.y), ltos(c.z)); }

vec2 scene(vec3 p)
{
    vec2 h = vec2(MAX_DIST, 0);

    {
        // Tunnel
        vec3 pp = p;
        pMod1(pp.z, 3.6);
        pModPolar(pp.xy, 6);
        {
            // Blocky tunnel 
            vec3 ppp = pp - vec3(0, 0, -0.5);
            float d = -fBox(ppp, vec3(2.15, 0.7, 5));
            h = d < h.x ? vec2(d, 1) : h;
        }

        {
            // Cutouts
            vec3 ppp = pp;
            ppp -= vec3(1.48, 0, 0.26);
            float d = -fBox(ppp, vec3(0.27, 1.1, 1.01));
            h = d > h.x ? vec2(d, 1) : h;
        }

    }

    {
        // Screens
        vec3 pp = p;
        pMod1(pp.z, 3.6);
        float d = -fBox(pp - vec3(0, 0, 0.26), vec3(2.17, 0.35, 1));
        h = d > h.x ? vec2(d, 2) : h;
    }

    {
        // Pipes
        vec3 pp = p;
        pR(pp.xy, 2);
        pModPolar(pp.xy, 6);
        pp -= vec3(1.81, -0.83, 4.89);
        pR(pp.yz, PI / 2);
        float d = fCylinder(pp, 0.16, 1000);
        h = d < h.x ? vec2(d, 3) : h;
    }

    return h;
}

vec2 march(vec3 ro, vec3 rd, float p, float m, int it)
{
    vec2 t = vec2(0.1);
    for (int i = 0; i < it; ++i) {
        vec2 h = scene(ro + rd * t.x);
        if (h.x < p || t.x > m)
            break;
        t.x += h.x;
        t.y = h.y;
    }
    if (t.x > m)
        t.y = 0;
    return t;
}

vec3 shade(vec3 p, vec3 n, vec3 v, Material m)
{
    // Calculate light position and distance
    vec3 l = LIGHT_POS - p;
    float r2 = dot(l, l);
    l /= sqrt(r2);

    // Check shadowing
    float t = march(p + n * 0.001, l, 0.001, MAX_DIST, 100).x;
    vec3 ambient = m.diffuse * LIGHT_AINT;
    vec3 diffuse, specular = vec3(0);
    if (t > sqrt(r2)) {
        // Do the Blinn-Phong
        vec3 h = normalize(v + l);
        float NoL = saturate(dot(n, l));
        float NoH = saturate(dot(n, h));
        diffuse = NoL * LIGHT_DINT * m.diffuse;
        specular = pow(pow(NoH, m.specularPow) * LIGHT_SINT * m.specular, vec3(10));
    }

    return ambient + (diffuse + specular) / r2 + m.emission;
}


vec3 normal(vec3 p, float m)
{
    vec3 e = vec3(0.0001, 0, 0);
    vec3 n = vec3(scene(vec3(p + e.xyy)).x - scene(vec3(p - e.xyy)).x,
                  scene(vec3(p + e.yxy)).x - scene(vec3(p - e.yxy)).x,
                  scene(vec3(p + e.yyx)).x - scene(vec3(p - e.yyx)).x);
    return normalize(n);
}

Material material(vec3 p, float i)
{
    Material m;
    m.diffuse = stol(vec3(dDiffuse));
    m.specular = stol(vec3(dSpecular));
    m.specularPow = dSpecularPow;
    m.emission = vec3(0);

    if (i == 1) {
        m.diffuse = stol(vec3(0.179, 0.03, 0)) * 2;
        m.specular = stol(vec3(0.745, 0.011, 0));
        m.specularPow = 64;
    } else if (i == 2) {
        m.diffuse = stol(vec3(0.06, 0.08, 0));
        m.specular = stol(vec3(0, 0.7, 0.07));
        m.specularPow = 32;
        if (p.z < 25) {
            float wform = sin(p.z * 10 - uTime) +
                          sin(p.z * 12 - uTime * 2) +
                          sin(p.z * 20 - uTime * 5) +
                          sin(p.z * 15 - uTime * 3);
            float dwf = saturate(pow(1 - abs(20 * p.y - wform), 5));
            vec3 indicator = vec3(0, 1, 0) * (1 / pow(abs(abs(wform) - 5), 7));
            m.emission = (stol(vec3(0.55, 0, 0.96)) * dwf + indicator) * sin(p.y * 300);
        }
    } else if (i == 3) {
        m.diffuse = vec3(0);
        m.specular = vec3(1);
        m.specularPow = 128;
        m.emission = stol(vec3(0, 1, 0.12)) * (1.1 + sin(uTime * 2 - p.z));
        if (p.z > 10)
            m.emission /= p.z / 2;
    }

    return m;
}

void main()
{
    // Generate neutral camera ray (+Z)
    vec2 uv = gl_FragCoord.xy / uRes.xy; // uv
    uv -= 0.5; // origin an center
    uv /= vec2(uRes.y / uRes.x, 1); // fix aspect ratio
    vec3 cd = normalize(vec3(uv, 0.7)); // pull ray
    pR(cd.yz, dCRot.y);
    pR(cd.xz, dCRot.x);
    vec3 cp = vec3(0, 0, 0.2) - dCPos;

    // Trace
    vec2 t = march(cp, cd, 0.001, MAX_DIST, 256);
    if (t.x > MAX_DIST) {
        fragColor = vec4(0);
        return;
    }
    vec3 p = cp + cd * t.x;

    // Shade
    Material m = material(p, t.y);
    vec3 color = shade(p, normal(p, t.y), -cd, m);

    fragColor = vec4(ltos(color), 1);
}
