#include <cmath>
#include <stdlib.h>
#include "J:/CodeBlocks/MinGW/include/glm/gtc/noise.hpp"

const float PI = 3.14159265358979323846;

inline float noise(float x, int seed)
{
    srand(pow(x, 1.12) * 61 - 1321 + seed);
    rand();
    return rand() / (float)RAND_MAX;
    //return noisetexture[int((x - floorf(x / 100) * 100) * 100 + y - floorf(y / 100) * 100)];
}

/*inline float noise(float x, float y, int seed)
{
    return noise(noise(x, seed) * 1000 + noise(y * 12, seed) * 877, seed);
}*/
inline float noise(float x_, float y_, int seed)
{
    int x = x_ * 100;
    int y = y_ * 100;
    int n = x + y * 57;
    n = (n << 13) ^ n + seed;
    int t = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return (float)t * 0.4656612873077392578125e-9;/// 1073741824.0);
}

inline float lerp(float a, float b, float t)
{
    return a + t * t * (3.f - 2.f * t) * (b - a);     //simple (0-derivative) cubic interpolation: cheap equivalent to cosine interpolation. (basis function: 3t^2 - 2t^3)
}
inline float frac(float x)
{
    return x - floorf(x);
}
inline float smoothnoise(float x, float y, int seed)
{
    return lerp(
                lerp(noise(floorf(x), floorf(y)    , seed), noise(floorf(x) + 1, floorf(y)    , seed), frac(x)),
                lerp(noise(floorf(x), floorf(y) + 1, seed), noise(floorf(x) + 1, floorf(y) + 1, seed), frac(x)),
                frac(y)
                );
}

float fbm(float x, float y, int octaves, float frequency, float persistence, int seed)
{
    if (octaves > 0)
        return smoothnoise(x, y, seed) + persistence * fbm(x * frequency, y * frequency, octaves - 1, frequency, persistence, seed);
    else
        return 0;
}

