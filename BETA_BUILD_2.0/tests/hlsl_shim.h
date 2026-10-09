// Minimal HLSL-in-C++ shim: enough to compile tt_cb.hlsl / tt_body.hlsl as C++
// and execute them, with real threads and a real barrier for compute groups.
#pragma once
#include <algorithm>
#include <atomic>
#include <barrier>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

typedef unsigned int uint;

struct float2 {
    float x = 0, y = 0;
    float2() {}
    float2(float a, float b) : x(a), y(b) {}
};
struct float4 {
    float x = 0, y = 0, z = 0, w = 0;
    float4() {}
    float4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {}
};
struct int3 {
    int x, y, z;
    int3(int a, int b, int c) : x(a), y(b), z(c) {}
};
struct uint3 {
    uint x = 0, y = 0, z = 0;
};
inline float4 operator*(float4 a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
inline float4 operator+(float4 a, float4 b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }

inline float saturate(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float clamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float4 lerp(float4 a, float4 b, float t) {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t), lerp(a.w, b.w, t)};
}
inline float max(float a, float b) { return a > b ? a : b; }
inline float min(float a, float b) { return a < b ? a : b; }
inline uint max(uint a, uint b) { return a > b ? a : b; }
inline uint min(uint a, uint b) { return a < b ? a : b; }
inline int max(int a, int b) { return a > b ? a : b; }
inline int min(int a, int b) { return a < b ? a : b; }
inline float dot(float2 a, float2 b) { return a.x * b.x + a.y * b.y; }
inline float length(float2 a) { return std::sqrt(dot(a, a)); }
using std::abs;
using std::ceil;
using std::cos;
using std::exp;
using std::floor;
using std::log10;
using std::pow;
using std::sin;
using std::sqrt;
using std::tanh;
inline uint reversebits(uint v) {
    uint r = 0;
    for (int i = 0; i < 32; i++)
        if (v & (1u << i)) r |= 1u << (31 - i);
    return r;
}
inline uint asuint(float f) {
    uint u;
    memcpy(&u, &f, 4);
    return u;
}
inline float asfloat(uint u) {
    float f;
    memcpy(&f, &u, 4);
    return f;
}
inline void InterlockedMax(uint& dst, uint v) {
    uint cur = __atomic_load_n(&dst, __ATOMIC_SEQ_CST);
    while (v > cur && !__atomic_compare_exchange_n(&dst, &cur, v, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
    }
}

template <typename T>
struct StructuredBuffer {
    std::vector<T>* data = nullptr;
    const T& operator[](uint i) const { return (*data).at(i); }
};
template <typename T>
struct RWStructuredBuffer {
    std::vector<T>* data = nullptr;
    T& operator[](uint i) const { return (*data).at(i); }
};
struct SamplerState {};
template <typename T>
struct Texture2D {
    std::vector<T>* data = nullptr;
    int w = 1, h = 1;
    T Load(int3 p) const { return (*data)[std::clamp(p.y, 0, h - 1) * w + std::clamp(p.x, 0, w - 1)]; }
    T SampleLevel(SamplerState, float2 uv, float) const {
        int x = std::clamp((int)(uv.x * w), 0, w - 1), y = std::clamp((int)(uv.y * h), 0, h - 1);
        return (*data)[y * w + x];
    }
};

#define TT_CBUFFER(n, r) struct n
#define TT_CBUFFER_END ;
#define REG(r)
#define SEM(s)
#define NUMTHREADS(x, y, z)
#define GROUPSHARED
#define NOINTERP
#define BARRIER tt_barrier->arrive_and_wait()
#define OUT(T) T&
