#include "utils.h"

namespace utils
{
void load_data_to_buffer(WGPUBuffer *buffer, void *data, const WGPUBufferDescriptor &buffer_desc, WGPUDevice device)
{
  *buffer = wgpuDeviceCreateBuffer(device, &buffer_desc);
  wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), *buffer, 0, data, buffer_desc.size);
}

WGPUBlendState wgpu_create_blend_state(bool enable_blend)
{
  WGPUBlendComponent blend_component_descriptor = {
    .operation = WGPUBlendOperation_Add,
  };

  if (enable_blend) {
    blend_component_descriptor.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_component_descriptor.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
  }
  else {
    blend_component_descriptor.srcFactor = WGPUBlendFactor_One;
    blend_component_descriptor.dstFactor = WGPUBlendFactor_Zero;
  }

  WGPUBlendState res {
    .color = blend_component_descriptor,
    .alpha = blend_component_descriptor,
  };

  return res;
}

void set_default_stencil_face_state(WGPUStencilFaceState &stencilFaceState) 
{
  stencilFaceState.compare = WGPUCompareFunction_Always;
  stencilFaceState.failOp = WGPUStencilOperation_Keep;
  stencilFaceState.depthFailOp = WGPUStencilOperation_Keep;
  stencilFaceState.passOp = WGPUStencilOperation_Keep;
}

void set_default_depth_stencil_state(WGPUDepthStencilState &depthStencilState) 
{
  depthStencilState.format = WGPUTextureFormat_Depth24Plus;
  depthStencilState.depthWriteEnabled = (WGPUOptionalBool)true;
  depthStencilState.depthCompare = WGPUCompareFunction_LessEqual;
  depthStencilState.stencilReadMask = 0xFFFFFFFF;
  depthStencilState.stencilWriteMask = 0xFFFFFFFF;
  depthStencilState.depthBias = 0;
  depthStencilState.depthBiasSlopeScale = 0;
  depthStencilState.depthBiasClamp = 0;
  set_default_stencil_face_state(depthStencilState.stencilFront);
  set_default_stencil_face_state(depthStencilState.stencilBack);
}
};