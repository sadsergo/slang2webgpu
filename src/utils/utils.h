#pragma once

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

namespace utils
{

typedef struct create_depth_stencil_state_desc_t 
{
  WGPUTextureFormat format;
  bool depth_write_enabled;
} create_depth_stencil_state_desc_t;

//  Copy data on GPU
void load_data_to_buffer(WGPUBuffer *buffer, void *data, const WGPUBufferDescriptor &buffer_desc, WGPUDevice device);
WGPUBlendState wgpu_create_blend_state(bool enable_blend);
void set_default_depth_stencil_state(WGPUDepthStencilState &depthStencilState);

};