#pragma once

#include "LiteMath.h"
#include <array>

using LiteMath::float4;
using LiteMath::float3; 
using LiteMath::float2;
using LiteMath::float4x4;

struct Vertex
{
  float3 pos;
  float3 normal;
  float3 color;
  float2 texCoord;
};

struct Mesh
{
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

struct Uniforms
{
  float4x4 projMtrx;
  float4x4 viewMtrx;
  float4x4 modelMtrx;
  float4 color;
  float time;
  float _pad[11];
};