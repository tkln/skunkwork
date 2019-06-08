#include <math.h>

float cosine_interpolation(float a, float b, float x)
{
    float ft = x * M_PI;
    float f = (1.0f - cosf(ft)) * 0.5f;
    return a * (1.0 - f) + b * f;
}

float linear_interpolation(float a, float b, float x)
{
    return a * (1.0 - x) + b * x;
}

float int_noise(uint32_t x, uint32_t seed)
{
    x = (x<<13) ^ x ^ seed;
    return (1.0f - ((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) /
           1073741824.0f);
}

float int_noise_2d(uint32_t x, uint32_t y, uint32_t seed)
{
    return int_noise(x + y * 8000, seed);
}

float int_noise_3d(uint32_t x, uint32_t y, uint32_t z, uint32_t seed)
{
    return int_noise(x + y * 8000 + z * 12345, seed);
}

float smooth_noise_2d(uint32_t x, uint32_t y, uint32_t seed)
{
    float corners = int_noise_2d(x - 1, y - 1, seed) + int_noise_2d(x + 1, y - 1, seed) +
                    int_noise_2d(x - 1, y + 1, seed) + int_noise_2d(x + 1, y + 1, seed);
    float sides = int_noise_2d(x - 1, y, seed) + int_noise_2d(x + 1, y, seed) +
                  int_noise_2d(x, y - 1, seed) + int_noise_2d(x, y + 1, seed);
    float center = int_noise_2d(x, y, seed);
    return corners / 16 + sides / 8 + center / 4;
}

float smooth_noise_3d(uint32_t x, uint32_t y, uint32_t z, uint32_t seed)
{
#if 1
    return int_noise_3d(x, y, z, seed);
#else
    /* 1 / 64 */
    float corners = int_noise_3d(x - 1, y - 1, z - 1, seed) +
                    int_noise_3d(x + 1, y - 1, z - 1, seed) +
                    int_noise_3d(x + 1, y - 1, z + 1, seed) +
                    int_noise_3d(x - 1, y - 1, z + 1, seed) +
                    int_noise_3d(x - 1, y + 1, z - 1, seed) +
                    int_noise_3d(x + 1, y + 1, z - 1, seed) +
                    int_noise_3d(x + 1, y + 1, z + 1, seed) +
                    int_noise_3d(x - 1, y + 1, z + 1, seed);

    /* 2 / 64 = 1 / 32 */
    float edges = int_noise_3d(x - 1, y + 0, z - 1, seed) +
                  int_noise_3d(x - 1, y - 1, z - 1, seed) +
                  int_noise_3d(x - 1, y + 0, z + 1, seed) +
                  int_noise_3d(x - 1, y + 1, z + 0, seed) +

                  int_noise_3d(x - 0, y - 1, z - 1, seed) +
                  int_noise_3d(x + 0, y - 1, z + 1, seed) +
                  int_noise_3d(x + 0, y + 1, z - 1, seed) +
                  int_noise_3d(x - 0, y + 1, z + 1, seed) +

                  int_noise_3d(x + 1, y + 0, z - 1, seed) +
                  int_noise_3d(x + 1, y - 1, z - 1, seed) +
                  int_noise_3d(x + 1, y + 0, z + 1, seed) +
                  int_noise_3d(x + 1, y + 1, z + 0, seed);


    /* 4 / 64 = 1 / 16 */
    float faces = int_noise_3d(x - 1, y + 0, z + 0, seed) +
                  int_noise_3d(x + 0, y + 0, z - 1, seed) +
                  int_noise_3d(x + 1, y + 0, z + 0, seed) +
                  int_noise_3d(x + 0, y + 0, z + 1, seed) +
                  int_noise_3d(x + 0, y - 1, z + 0, seed) +
                  int_noise_3d(x + 0, y + 1, z + 0, seed);

    /* 8 / 64 = 1 / 8 */
    float center = int_noise_3d(x, y, z, seed);

    return corners / 64.0f + edges / 32.0f + faces / 16.0f + center / 8.0f;
#endif
}

float interpolated_noise_3d(float x, float y, float z, uint32_t seed)
{
    int32_t ix = x;
    int32_t iy = y;
    int32_t iz = z;
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;

    float v0, v1, v2, v3, v4, v5, v6, v7;
    v0 = smooth_noise_3d(ix + 0, iy + 0, iz + 0, seed);
    v1 = smooth_noise_3d(ix + 1, iy + 0, iz + 0, seed);
    v2 = smooth_noise_3d(ix + 0, iy + 1, iz + 0, seed);
    v3 = smooth_noise_3d(ix + 1, iy + 1, iz + 0, seed);
    v4 = smooth_noise_3d(ix + 0, iy + 0, iz + 1, seed);
    v5 = smooth_noise_3d(ix + 1, iy + 0, iz + 1, seed);
    v6 = smooth_noise_3d(ix + 0, iy + 1, iz + 1, seed);
    v7 = smooth_noise_3d(ix + 1, iy + 1, iz + 1, seed);


#if 0
    float i0, i1, i2, i3;
    i0 = cosine_interpolation(v0, v1, fx);
    i1 = cosine_interpolation(v2, v3, fx);
    i2 = cosine_interpolation(v4, v5, fx);
    i3 = cosine_interpolation(v6, v7, fx);

    float i4, i5;
    i4 = cosine_interpolation(i0, i1, fy);
    i5 = cosine_interpolation(i2, i3, fy);

    return cosine_interpolation(i4, i5, fz);
#else
    float i0, i1, i2, i3;
    i0 = linear_interpolation(v0, v1, fx);
    i1 = linear_interpolation(v2, v3, fx);
    i2 = linear_interpolation(v4, v5, fx);
    i3 = linear_interpolation(v6, v7, fx);

    float i4, i5;
    i4 = linear_interpolation(i0, i1, fy);
    i5 = linear_interpolation(i2, i3, fy);

    return linear_interpolation(i4, i5, fz);
#endif
}

float perlin_noise_3d(float x, float y, float z, float p, int n, uint32_t seed)
{
    float total = 0;
    int i;
    for (i = 0; i < n; ++i) {
        float freq = powf(2.0, i);
        float amp = powf(p, i);
        total += interpolated_noise_3d(x * freq, y * freq, z * freq, seed) * amp;
    }
    return total;
}
