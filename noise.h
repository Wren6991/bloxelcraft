#ifndef _NOISE_H_INCLUDED_
#define _NOISE_H_INCLUDED_

inline float noise(float x, int seed);
inline float noise(float x, float y, int seed);

inline float lerp(float a, float b, float t);
inline float frac(float x);
inline float smoothnoise(float x, float y, int seed);

float fbm(float x, float y, int octaves, float frequency, float persistence, int seed);

#endif //_NOISE_H_INCLUDED_
