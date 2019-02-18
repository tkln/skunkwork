#version 410

#include "uniforms.glsl"

uniform vec3 dColor;
uniform vec3 dCPos;
uniform vec2 dCDir;
uniform vec3 dLColor;
uniform float dRoughness;
uniform float dMetalness;

out vec4 fragColor;

const int SAMPLES_PER_PIXEL = 20;
const int MAX_BOUNCES = 3;
const float EPSILON = 0.01;

// From hg_sdf
#define PI 3.14159265
#define saturate(x) clamp(x, 0, 1)
#define square(x) x * x
void pR(inout vec2 p, float a) {
    p = cos(a)*p + sin(a)*vec2(p.y, -p.x);
}

// From iq
float seed; //seed initialized in main
float rnd() { return fract(sin(seed++)*43758.5453123); }

// Conversions from sRGB to linear space
#define stol1(x) (x <= 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4))
#define stol3(x, y, z) vec3(stol1(x), stol1(y), stol1(z))

struct AreaLight {
    mat4 toWorld;
    vec2 size;
    vec3 E;
};

struct Material {
    vec3 albedo;
    float roughness;
    float metalness;
    vec3 emission;
};

struct Ray {
    vec3 o;
    vec3 d;
    float t;
};

struct Hit {
    bool hit;
    vec3 position;
    vec3 normal;
    Material material;
};

struct Sphere {
    vec3 center;
    float r;
};

struct RDir {
    vec3 d;
    float pdf;
};

const int NUM_OBJECTS = 7;
const Sphere objects[] = Sphere[](
    Sphere(vec3(-1.5, -3,.1), 2),
    Sphere(vec3(1, -4, -2), 1),
    Sphere(vec3(0, -10005, 0), 10000),
    Sphere(vec3(0, 0, 10005), 10000),
    Sphere(vec3(0, 10005, 0), 10000),
    Sphere(vec3(-10005, 0, 0), 10000),
    Sphere(vec3(10005, 0, 0), 10000)
);
const vec3 COLORS[] = vec3[](
    stol3(0, 0, 1),
    stol3(1, 0.863, 0.616),
    vec3(180) / vec3(255),
    vec3(180) / vec3(255),
    vec3(180) / vec3(255),
    vec3(180,0,0) / vec3(255),
    vec3(0,180,0) / vec3(255)
);


const int NUM_LIGHTS = 1;
const AreaLight lights[] = AreaLight[](
    AreaLight(mat4(1, 0, 0, 0,
                   0, 1, 0, 0,
                   0, 0, 1, 0,
                   0, 5, 0, 1),
              vec2(1),
              vec3(0.85, 0.8, 0.4) * vec3(30))
);

mat3 formBasis(vec3 n)
{
    // Make vector q that is non-parallel to n
    vec3 q = n;
    vec3 aq = abs(q);
    if (aq.x <= aq.y && aq.x <= aq.z) {
        q.x = 1.f;
    } else if (aq.y <= aq.x && aq.y <= aq.z) {
        q.y = 1.f;
    } else {
        q.z = 1.f;
    }

    // Generate two vectors perpendicular to n
    vec3 t = normalize(cross(q, n));
    vec3 b = normalize(cross(n, t));

    // Construct the rotation matrix
    mat3 m;
    m[0] = t;
    m[1] = b;
    m[2] = n;
    return m;
}

// Sampling
vec3 cosineSampleHemisphere() {
    // Get random reflection direction
    vec2 p;
    float p_sqr;
    do {
        p = vec2(rnd(), rnd()) * 2 - 1;
        p_sqr = dot(p, p);
    } while (p_sqr > 1);
    vec3 rd = vec3(p.x, p.y, sqrt(1 - p_sqr));
    return rd;
}

// Shading
vec3 lambertBRFD(vec3 albedo)
{
    return albedo / PI;
}

float ggx(float NoH, float rough)
{
    float a2 = rough * rough;
    a2 *= a2;
    float denom = NoH * NoH * (a2 - 1) + 1;
    return a2 / (PI * denom * denom);
}

vec3 schlickFresnel(float VoH, vec3 f0)
{
    return f0 + (1 - f0) * pow(1 - VoH, 5);
}

float schlick_ggx(float NoL, float NoV, float rough)
{
    float k = (rough + 1);
    k *= k * 0.125;
    float gl = NoL / (NoL * (1 - k) + k);
    float gv = NoV / (NoV * (1 - k) + k);
    return gl * gv;
}

vec3 cookTorranceBRDF(float NoL, float NoV, float NoH, float VoH, vec3 F, float rough)
{
    vec3 DFG = ggx(NoH, rough) * F * schlick_ggx(NoL, NoV, rough);
    float denom = 4 * NoL * NoV + 0.0001;
    return DFG / denom;
}

vec3 evalBRDF(vec3 n, vec3 v, vec3 l, Material m)
{
    // Common dot products
    float NoV = saturate(dot(n, v));
    float NoL = saturate(dot(n, l));
    vec3 h = normalize(v + l);
    float NoH = saturate(dot(n, h));
    float VoH = saturate(dot(v, h));

    // Use standard approximation of default fresnel
    vec3 f0 = mix(vec3(0.04), m.albedo, m.metalness);
    vec3 F = schlickFresnel(VoH, f0);

    // Diffuse amount
    vec3 Kd = (1 - F) * (1 - m.metalness);

    return (Kd * lambertBRFD(m.albedo) + cookTorranceBRDF(NoL, NoV, NoH, VoH, F, m.roughness)) * NoL;
}


float intersect(Ray r, Sphere s)
{
    vec3 L = s.center - r.o;
    float tc = dot(L, r.d);
    float d2 = dot(L, L) - tc * tc;
    float r2 = s.r * s.r;
    if (d2 > r2)
        return r.t;

    float tlc = sqrt(r2 - d2);
    float t0 = tc - tlc;
    float t1 = tc + tlc;
    if (t0 > t1) {
        float tmp = t0;
        t0 = t1;
        t1 = tmp;
    }
    if (t0 < 0) {
        if (t1 < 0)
            return r.t;
        return t1;
    }
    return t0;
}

Material evalMaterial(vec3 p, int i)
{
    Material m;
    m.albedo = vec3(1,0,1);
    m.roughness = 1;
    m.metalness = 0;
    m.emission = vec3(1,0,1);
    if (i >= 0) {
        m.albedo = COLORS[i];
        m.emission = vec3(0);
        if (i == 0) {
            m.roughness = 0.3;
            m.metalness = 0;
        } else if (i == 1) {
            m.roughness = 0.4;
            m.metalness = 1;
        } else if (i == 4 && all(lessThan(abs(p.xz), lights[0].size))) {
            m.emission = lights[0].E;
        }
    }
    return m;
}

Hit traceRay(Ray r)
{
    int object = -1;
    float t = r.t;
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        Sphere s = objects[i];
        float nt = intersect(r, s);
        if (nt < t) {
            t = nt;
            object = i;
        }
    }
    vec3 position = vec3(0);
    vec3 normal = vec3(0);
    if (object >= 0) {
        position = r.o + t * r.d;
        normal = normalize(position - objects[object].center);
    }
    return Hit(object >= 0, position, normal, evalMaterial(position, object));
}

vec3 getViewRay(vec2 px, float hfov)
{
    vec2 xy = px.xy - uRes * 0.5;
    float z = uRes.y / tan(radians(hfov));
    vec3 d = normalize(vec3(xy, z));
    pR(d.yz, dCDir.y);
    pR(d.xz, dCDir.x);
    return d;
}

vec3 tracePath(vec2 px)
{
    vec3 ei = vec3(0);
    for (int j = 0; j < SAMPLES_PER_PIXEL; ++j) {
        // Generate ray
        vec2 sample_px = gl_FragCoord.xy + vec2(rnd(), rnd());
        Ray r = Ray(dCPos, getViewRay(sample_px, 45), 100);


        vec3 throughput = vec3(1);
        for (int bounce = 0; bounce < MAX_BOUNCES; ++bounce) {
            Hit hit = traceRay(r);
            // Cut ray on miss, backward hit or being outside the box
            if (!hit.hit || dot(hit.normal, r.d) > 0 || hit.position.z < -5)
                break;

            // Collect common info
            Material m = hit.material;
            vec3 n = hit.normal;
            vec3 p = hit.position + hit.normal * 0.01;

            // Add material emission
            if (bounce == 0)
                ei += throughput * m.emission;

            // Sample lights
            for (int i = 0; i < NUM_LIGHTS; ++i) {
                AreaLight light = lights[i];
                float pdf = 1 / (4 * light.size.x * light.size.y);
                mat4 S = mat4(light.size.x,            0, 0, 0,
                                        0, light.size.y, 0, 0,
                                        0,            0, 1, 0,
                                        0,            0, 0, 1);
                mat4 M = light.toWorld * S;
                vec3 pL = (M * vec4(vec2(rnd(), rnd()) * 2 - 1, 0, 1)).xyz;

                vec3 toLight = pL - p;
                Ray sr;
                sr.o = p;
                sr.d = normalize(toLight);
                sr.t = length(toLight) - EPSILON;
                Hit sh = traceRay(sr);
                if (!sh.hit) {
                    float r2 = dot(toLight, toLight);
                    vec3 lN = vec3(0, -1, 0); // TODO: generic
                    vec3 le = lights[i].E;
                    ei += throughput * evalBRDF(hit.normal, -r.d, sr.d, m) * le / (r2 * pdf);
                }
            }

            // Get direction for next reflection ray
            vec3 rd = cosineSampleHemisphere();
            // Rotate to normal direction and scale
            rd = normalize(formBasis(n) * rd);
            // PDF for outgoing direction
            float pdf = dot(n, rd) / PI; // this Seems Right^tm and could be simplified to just * PI in throughput
            // TODO: Multiple importance sampling on diffuse and specular?
            throughput *= evalBRDF(hit.normal, -r.d, rd, m) / pdf;
            r.d = rd;
            r.o = p;
        }
    }
    return ei / SAMPLES_PER_PIXEL;
}

void main()
{
    // Cinema bars
    if (abs(gl_FragCoord.y / uRes.y - 0.5) > 0.375) {
        fragColor = vec4(0);
        return;
    }

    // Reseed by iq
    seed = uTime + gl_FragCoord.y * gl_FragCoord.x / uRes.x + gl_FragCoord.y / uRes.y;

    vec3 color = tracePath(gl_FragCoord.xy);
    fragColor = vec4(pow(color, vec3(0.4545)), 1); // Quick and dirty gc
}
