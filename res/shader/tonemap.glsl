// sRGB, linear space conversions
float stol(float x) { return (x <= 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4)); }
vec3 stol(vec3 c) { return vec3(stol(c.x), stol(c.y), stol(c.z)); }

// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
// https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting
vec3 Uncharted2Tonemap(vec3 color)
{
    float A = 0.15; // Shoulder strength
    float B = 0.50; // Linear strength
    float C = 0.10; // Linear angle
    float D = 0.20; // Toe strength
    float E = 0.02; // Toe numerator
    float F = 0.30; // Toe denominator
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec3 tonemap(vec3 color)
{
    float exposure = 1.0;
    float gamma = 2.2;
    float linearWhite = 11.2;
    vec3 outcol = Uncharted2Tonemap(color * exposure);
    outcol /= Uncharted2Tonemap(vec3(linearWhite));
    return pow(outcol, vec3(1 / gamma));
}
