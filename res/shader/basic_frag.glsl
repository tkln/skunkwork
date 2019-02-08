#version 410

#include "uniforms.glsl"
#include "hg_sdf.glsl"

uniform vec3 dColor;
uniform vec3 dCPos;
uniform vec2 dCDir;

out vec4 fragColor;

const int SAMPLES_PER_PIXEL = 10;
const int MAX_BOUNCES = 2;

// From iq
float seed; //seed initialized in main
float rnd() { return fract(sin(seed++)*43758.5453123); }

struct Ray {
    vec3 o;
    vec3 d;
    float t;
};

struct Hit {
    bool hit;
    vec3 position;
    vec3 normal;
    vec3 diffuse;
    vec3 emission;
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
    Sphere(vec3(-0.15, -0.3, 0.1), 0.2),
    Sphere(vec3(0.1, -0.4, -0.2), 0.1),
    Sphere(vec3(0, -1000.5, 0), 1000),
    Sphere(vec3(0, 0, 1000.5), 1000),
    Sphere(vec3(0, 1000.5, 0), 1000),
    Sphere(vec3(-1000.5, 0, 0), 1000),
    Sphere(vec3(1000.5, 0, 0), 1000)
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

RDir sampleReflection(vec3 n) {
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

vec3 getViewRay(vec2 px, float fov)
{
    vec2 xy = px.xy - uRes * 0.5;
    float z = uRes.y / tan(radians(fov * 0.5));
    return normalize(vec3(xy, z));
}

Hit traceRay(Ray r)
{
    int object = -1;
    float t = 100;
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
    vec3 color = vec3(0);
    vec3 emission = vec3(0);
    if (object >= 0) {
        position = r.o + r.d * t;
        normal = normalize(r.o + r.d * t - objects[object].center);
        switch (object) {
            case 0:
            case 1:
                color = vec3(80) / vec3(255);
                break;
            case 2:
            case 3:
                color = vec3(180) / vec3(255);
                break;
            case 4:
                if (length(position.xz) < 0.2)
                    emission = vec3(5);
                break;
            case 5:
                color = vec3(180, 0, 0) / vec3(255);
                break;
            case 6:
                color = vec3(0, 180, 0) / vec3(255);
                break;
            default:
                emission = vec3(1,1,0);
                break;
        }
    }
    return Hit(object >= 0, position, normal, color / PI, emission);
}

vec3 tracePath(vec2 px)
{
    vec3 ei = vec3(0);
    for (int j = 0; j < SAMPLES_PER_PIXEL; ++j) {
        vec3 throughput = vec3(1);
        vec2 sample_px = gl_FragCoord.xy + vec2(rnd(), rnd());
        Ray r = Ray(dCPos, getViewRay(sample_px, 90), 100);
        pR(r.d.yz, dCDir.x);
        pR(r.d.xz, dCDir.y);
        for (int i = 0; i < MAX_BOUNCES; ++i) {
            Hit h = traceRay(r);
            if (h.hit) {
                ei += h.emission * throughput;
                r.o = h.position + h.normal * 0.001;
                RDir rd = sampleReflection(h.normal);
                r.d = rd.d;
                float cosTheta = max(dot(h.normal, rd.d), 0);
                throughput *= h.diffuse * cosTheta / rd.pdf;
            } else
                break;
        }
    }
    return ei / SAMPLES_PER_PIXEL;
}

void main()
{
    seed = uTime + gl_FragCoord.y * gl_FragCoord.x / uRes.x + gl_FragCoord.y / uRes.y;
    fragColor = vec4(tracePath(gl_FragCoord.xy), 1);
    if (uTime < 0) fragColor = vec4(0);
}
