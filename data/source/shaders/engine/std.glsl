//
// created by : Timothee Feuillet
// date: 2022-8-7
//
//
// Copyright (c) 2022 Timothee Feuillet
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


const float k_pi = 3.14159265358979323846;
const float k_2pi = k_pi * 2.0;



float saturate(float x) { return clamp(x, 0, 1); }
float2 saturate(float2 x) { return clamp(x, (0).xx, (1).xx); }
float3 saturate(float3 x) { return clamp(x, (0).xxx, (1).xxx); }
float4 saturate(float4 x) { return clamp(x, (0).xxxx, (1).xxxx); }


// asuint, asint, asfloat
// (shorter than [x]BitsTo[y], and fixes the missing [u]intBitsTo[u]int)

uint asuint(int x) { return floatBitsToUint(intBitsToFloat(x)); }
uint asuint(float x) { return floatBitsToUint(x); }

uint2 asuint(int2 x) { return floatBitsToUint(intBitsToFloat(x)); }
uint2 asuint(float2 x) { return floatBitsToUint(x); }

uint3 asuint(int3 x) { return floatBitsToUint(intBitsToFloat(x)); }
uint3 asuint(float3 x) { return floatBitsToUint(x); }

uint4 asuint(int4 x) { return floatBitsToUint(intBitsToFloat(x)); }
uint4 asuint(float4 x) { return floatBitsToUint(x); }


int asint(uint x) { return floatBitsToInt(uintBitsToFloat(x)); }
int asint(float x) { return floatBitsToInt(x); }

int2 asint(uint2 x) { return floatBitsToInt(uintBitsToFloat(x)); }
int2 asint(float2 x) { return floatBitsToInt(x); }

int3 asint(uint3 x) { return floatBitsToInt(uintBitsToFloat(x)); }
int3 asint(float3 x) { return floatBitsToInt(x); }

int4 asint(uint4 x) { return floatBitsToInt(uintBitsToFloat(x)); }
int4 asint(float4 x) { return floatBitsToInt(x); }


float asfloat(int x) { return intBitsToFloat(x); }
float asfloat(uint x) { return uintBitsToFloat(x); }

float2 asfloat(int2 x) { return intBitsToFloat(x); }
float2 asfloat(uint2 x) { return uintBitsToFloat(x); }

float3 asfloat(int3 x) { return intBitsToFloat(x); }
float3 asfloat(uint3 x) { return uintBitsToFloat(x); }

float4 asfloat(int4 x) { return intBitsToFloat(x); }
float4 asfloat(uint4 x) { return uintBitsToFloat(x); }


// HLSL compat:

#define lerp mix


// Useful pow replacements

float pow2(float x) { return x * x; }
float2 pow2(float2 x) { return x * x; }
float3 pow2(float3 x) { return x * x; }
float4 pow2(float4 x) { return x * x; }

float pow3(float x) { return x * x * x; }
float2 pow3(float2 x) { return x * x * x; }
float3 pow3(float3 x) { return x * x * x; }
float4 pow3(float4 x) { return x * x * x; }

float pow4(float x) { return pow2(pow2(x)); }
float2 pow4(float2 x) { return pow2(pow2(x)); }
float3 pow4(float3 x) { return pow2(pow2(x)); }
float4 pow4(float4 x) { return pow2(pow2(x)); }

float pow5(float x) { return pow4(x) * x; }
float2 pow5(float2 x) { return pow4(x) * x; }
float3 pow5(float3 x) { return pow4(x) * x; }
float4 pow5(float4 x) { return pow4(x) * x; }

float pow6(float x) { return pow3(pow2(x)); }
float2 pow6(float2 x) { return pow3(pow2(x)); }
float3 pow6(float3 x) { return pow3(pow2(x)); }
float4 pow6(float4 x) { return pow3(pow2(x)); }
