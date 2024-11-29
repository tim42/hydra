//
// created by : Timothée Feuillet
// date: 2022-8-6
//
//
// Copyright (c) 2022 Timothée Feuillet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include "std.glsl"

// Functions in this header:
//  invwk_rnd : n -> n noise function (seed update)
//  invwk_rnd[m] : n -> m (m > 1) noise function (seed update)
//
//  rnd_seed : n -> 1, no seed update
//  rnd_merge : n -> 1, seed update
//  rnd_split[m] : 1 -> m (m > 1), no seed update
//
//  to_unorm : convert a uint[m] to a float[m] with each components in the range of [0, 1[
//             NOTE: the components are independent, normalizing the result will not give correct results
//  to_snorm : convert a uint[m] to a float[m] with each components in the range of ]-1, 1[
//             NOTE: the components are independent, normalizing the result will not give correct results
//  to_hnorm : convert a uint[m] to a float[m+1], the return is normalized
//             the distribution is uniform along the surface (unit circle, unit sphere)

// n -> n, inout

uint invwk_rnd(inout uint x)
{
  x += ((x * x) | 5);
  return x;
}

uint64_t invwk_rnd(inout uint64_t x)
{
  x += ((x * x) | 5);
  return x;
}

uint2 invwk_rnd(inout uint2 x)
{
  x += ((x * x) | 5);
  return x;
}

uint3 invwk_rnd(inout uint3 x)
{
  x += ((x * x) | 5);
  return x;
}

uint4 invwk_rnd(inout uint4 x)
{
  x += ((x * x) | 5);
  return x;
}

// n -> 1, no inout

uint64_t rnd_seed(uint64_t x)
{
  return invwk_rnd(x.x);
}

uint rnd_seed(uint x)
{
  return invwk_rnd(x.x);
}

uint rnd_seed(uint2 x)
{
  return invwk_rnd(x.x) * invwk_rnd(x.y);
}

uint rnd_seed(uint3 x)
{
  return invwk_rnd(x.x) * invwk_rnd(x.y) * invwk_rnd(x.z);
}

uint rnd_seed(uint4 x)
{
  return invwk_rnd(x.x) * invwk_rnd(x.y) * invwk_rnd(x.z) * invwk_rnd(x.w);
}

// n -> 1, inout

uint rnd_merge(inout uint2 x)
{
  return invwk_rnd(x.x) * invwk_rnd(x.y);
}

uint rnd_merge(inout uint3 x)
{
  return invwk_rnd(x.x) * invwk_rnd(x.y) * invwk_rnd(x.z);
}

uint rnd_merge(inout uint4 x)
{
  return invwk_rnd(x.x) * invwk_rnd(x.y) * invwk_rnd(x.z) * invwk_rnd(x.w);
}

// n -> m, inout

uint2 invwk_rnd2(inout uint x)
{
  return uint2(invwk_rnd(x), invwk_rnd(x));
}

uint3 invwk_rnd3(inout uint x)
{
  return uint3(invwk_rnd(x), invwk_rnd(x), invwk_rnd(x));
}

uint4 invwk_rnd4(inout uint x)
{
  return uint4(invwk_rnd(x), invwk_rnd(x), invwk_rnd(x), invwk_rnd(x));
}



uint3 invwk_rnd3(inout uint2 x)
{
  return uint3(invwk_rnd(x.x), invwk_rnd(x.y), rnd_merge(x));
}

uint4 invwk_rnd4(inout uint2 x)
{
  return uint4(invwk_rnd(x.x), invwk_rnd(x.y), invwk_rnd(x.x), invwk_rnd(x.y));
}


uint2 invwk_rnd2(inout uint3 x)
{
  return uint2(rnd_merge(x.xz), rnd_merge(x.yz));
}

uint4 invwk_rnd4(inout uint3 x)
{
  return uint4(invwk_rnd(x.x), invwk_rnd(x.y), invwk_rnd(x.z), rnd_merge(x));
}

uint2 invwk_rnd2(inout uint4 x)
{
  return uint2(rnd_merge(x.xy), rnd_merge(x.zw));
}

uint3 invwk_rnd3(inout uint4 x)
{
  return uint3(rnd_merge(x.xw), rnd_merge(x.yw), rnd_merge(x.zw));
}

// n -> m, no inout

uint2 rnd_split2(uint x)
{
  return uint2(invwk_rnd(x), invwk_rnd(x));
}

uint3 rnd_split3(uint x)
{
  return uint3(invwk_rnd(x), invwk_rnd(x), invwk_rnd(x));
}

uint4 rnd_split4(uint x)
{
  return uint4(invwk_rnd(x), invwk_rnd(x), invwk_rnd(x), invwk_rnd(x));
}



uint3 rnd_split3(uint2 x)
{
  return uint3(invwk_rnd(x.x), invwk_rnd(x.y), rnd_merge(x));
}

uint4 rnd_split4(uint2 x)
{
  return uint4(invwk_rnd(x.x), invwk_rnd(x.y), invwk_rnd(x.x), invwk_rnd(x.y));
}


uint2 rnd_split2(uint3 x)
{
  return uint2(rnd_merge(x.xz), rnd_merge(x.yz));
}

uint4 rnd_split4(uint3 x)
{
  return uint4(invwk_rnd(x.x), invwk_rnd(x.y), invwk_rnd(x.z), rnd_merge(x));
}

uint2 rnd_split2(uint4 x)
{
  return uint2(rnd_merge(x.xy), rnd_merge(x.zw));
}

uint3 rnd_split3(uint4 x)
{
  return uint3(rnd_merge(x.xw), rnd_merge(x.yw), rnd_merge(x.zw));
}


// full precision float conversions: (all of them are uniform in their respective spaces)
//   unorm : [0, 1[  (usage: 23bit)
//   snorm: ]-1, 1[  (usage: 24bit)
//
//   hnorm: [-1, 1], but length is 1. (usage: 24bit on x, 23bit on y (for 3d only))

float to_unorm(uint x)
{
  return asfloat((x >> 9) | 0x3f800000) - 1.0;
}

float2 to_unorm(uint2 x)
{
  return asfloat((x >> 9) | 0x3f800000) - 1.0;
}

float3 to_unorm(uint3 x)
{
  return asfloat((x >> 9) | 0x3f800000) - 1.0;
}

float4 to_unorm(uint4 x)
{
  return asfloat((x >> 9) | 0x3f800000) - 1.0;
}


// snorm conversion with 24bit randomness (unrom * 2 - 1 has 23bit randomness)
// cost little a bit more than unorm * 2 - 1

float to_snorm(uint x)
{
  uint base = 0x3f800000 | (x & 0x80000000);
  return asfloat(((x & 0x7FFFFFFF) >> 8) | base) - asfloat(base);
}

float2 to_snorm(uint2 x)
{
  uint2 base = 0x3f800000 | (x & 0x80000000);
  return asfloat(((x & 0x7FFFFFFF) >> 8) | base) - asfloat(base);
}

float3 to_snorm(uint3 x)
{
  uint3 base = 0x3f800000 | (x & 0x80000000);
  return asfloat(((x & 0x7FFFFFFF) >> 8) | base) - asfloat(base);
}

float4 to_snorm(uint4 x)
{
  uint4 base = 0x3f800000 | (x & 0x80000000);
  return asfloat(((x & 0x7FFFFFFF) >> 8) | base) - asfloat(base);
}


// random point on the 2D unit circle (hnorm2) / surface of the 3D unit sphere (hnorm3).
// the random is uniform, and the return is a normalized vector

float2 to_hnorm2(uint x)
{
  return sin(vec2(k_pi2, 0) + to_unorm(x.x) * k_2pi);
}

float3 to_hnorm3(uint2 x)
{
  float rx = to_snorm(x.x);
  return float3(rx, sqrt(1.0 - rx * rx) * sin(vec2(k_pi2, 0) + to_unorm(x.y) * k_2pi));
}
