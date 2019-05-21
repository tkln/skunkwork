// 3D noise function with tweaks (IQ, Shane)
// Range [0, 1]
// https://www.shadertoy.com/view/lstGRB
float noise(vec3 p)
{
    // Stride parameters
    const vec3 s = vec3(7, 17, 13);
    // Cell id
    vec3 ip = floor(p);
    // IQ's magic
    vec4 h = vec4(0, s.yz, s.y + s.z) + dot(ip, s);
    // Cell fraction
    p -= ip;
    // Cubic smoothing
    p = p*p*(3 - 2 * p);
    // Generate noise values for cube corners and mix along axes
    h = mix(fract(sin(h) * 43758.5453), fract(sin(h + s.x) * 43758.5453), p.x);
    h.xy = mix(h.xz, h.yw, p.y);
    return mix(h.x, h.y, p.z);
}

// Noise with smooth animation
float snoise(vec3 p)
{
    return mix(noise(p - 1.5), noise(p), 0.5);
}

// Simplified from Physically Based Rendering by Pharr et. al.
float fbm(vec3 p, float omega, int octaves) {
    float sum = 0, lambda = 1, o = 1;
    for (int i = 0; i < octaves; ++i) {
        sum += o * snoise(lambda * p);
        lambda *= 1.99;
        o *= omega;
    }
    return sum;
}
