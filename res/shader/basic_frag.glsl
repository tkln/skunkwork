#version 410

#include "uniforms.glsl"

uniform vec3 dColor;
uniform vec3 dCPos;
uniform vec2 dCDir;
uniform vec3 dLColor;
uniform float dRoughness;

out vec4 fragColor;

const int SAMPLES_PER_PIXEL = 100;
const int MAX_BOUNCES = 5;
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
    vec3(80) / vec3(255),
    vec3(180) / vec3(255),
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
              vec2(2),
              vec3(0.85, 0.8, 0.4) * vec3(15))
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

// TODO: fix me to match the naive sampling visually
vec3 uniformSampleHemisphere()
{
    vec2 u = vec2(rnd(), rnd());
    float z = u.x;
    float r = sqrt(saturate(1 - z * z));
    float phi = 2 * PI * u.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

float uniformHemispherePDF()
{
    return 1 / (2 * PI);
}

RDir sampleDiffuseReflection(vec3 n) {
    // Get random reflection direction
    vec2 p;
    float p_sqr;
    do {
        p = vec2(rnd(), rnd()) * 2 - 1;
        p_sqr = dot(p, p);
    } while (p_sqr > 1);
    RDir rd;
    rd.d = vec3(p.x, p.y, sqrt(1 - p_sqr));

    // Rotate to normal direction and scale
    rd.d = normalize(formBasis(n) * rd.d);
    rd.pdf = dot(rd.d, n) / PI; // this Seems Right^tm
    return rd;
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
        m.albedo = COLORS[i] / PI;
        m.emission = vec3(0);
        if (i == 4 && all(lessThan(abs(p.xz), lights[0].size)))
            m.emission = lights[0].E;
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
            Hit h = traceRay(r);
            // Cut ray on miss, backward hit or being outside the box
            if (!h.hit || dot(h.normal, r.d) > 0 || h.position.z < -5)
                break;

            r.o = h.position + h.normal * 0.01;
            // Add material emission
            ei += throughput * h.material.emission;

            // Get direction for next reflection ray
            RDir rd = sampleDiffuseReflection(h.normal);
            r.d = rd.d;
            float cosTheta = max(dot(h.normal, rd.d), 0);
            throughput *= h.material.albedo * cosTheta / rd.pdf;

            /*
            // TODO: fix me to match the naive sampling visually
            // Get direction for next reflection ray
            r.d = normalize(formBasis(h.normal) * uniformSampleHemisphere());
            float cosTheta = saturate(dot(h.normal, r.d));
            float pdf = uniformHemispherePDF();
            throughput *= h.diffuse * cosTheta / pdf;
            */
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
