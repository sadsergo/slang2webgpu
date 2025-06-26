#include "render.h"
#include <iostream>

#define UNUSED(x) (void)(x)
#define WEBGPU_STR(str) WGPUStringView{ str, sizeof(str) - 1 }
namespace WGPU
{
  std::string readFile(const char* path) {
      std::ifstream file(path, std::ios::binary);
      return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  }

  void RasterizationRenderAPI::Draw(uint8_t* data) const
  {

  }

  void RasterizationRenderAPI::Init(std::shared_ptr<WGPUDevice> device, std::shared_ptr<WGPUQueue> queue)
  {
    this->device = device;
    this->queue = queue;
    
    //  Load the shader module
    std::string shader_code = readFile("shaders/rasterization.wgsl");

    const WGPUChainedStruct tmp1 = { .sType = WGPUSType_ShaderSourceWGSL };
    
    const WGPUShaderSourceWGSL tmp2 = {
      .chain = tmp1,
      .code = {shader_code.c_str(), WGPU_STRLEN},
    };

    const WGPUShaderModuleDescriptor tmp3 = {
      .nextInChain = (const WGPUChainedStruct *)&tmp2,
      .label = WEBGPU_STR("Rasterization shader module")
    };

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(*this->device, &tmp3);

    WGPUPipelineLayoutDescriptor pipelineLayoutDesc{};
    pipelineLayoutDesc.label = WEBGPU_STR("Rasterization pipeline layout desc");

    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(*device, &pipelineLayoutDesc);

    WGPUVertexState vertex_state = { .module = shader_module, .entryPoint = WEBGPU_STR("vs_main") };
    const WGPUColorTargetState tmp4 = {
      .format = WGPUTextureFormat_RGBA8Unorm,
      .writeMask = WGPUColorWriteMask_All,
    };
    const WGPUColorTargetState targets[] = { tmp4 };
    const WGPUFragmentState fragment_state = { .module = shader_module, .entryPoint = WEBGPU_STR("fs_main"), .targetCount = 1, .targets = targets };

    const WGPUPrimitiveState prim_state = { .topology = WGPUPrimitiveTopology_TriangleList };
    const WGPUMultisampleState multisample_state = { .count = 1, .mask = 0xFFFFFFFF };

    WGPURenderPipelineDescriptor renderPipelineDesc{};
    renderPipelineDesc.label = WEBGPU_STR("rasterization pipeline desc");
    renderPipelineDesc.layout = pipeline_layout;
    renderPipelineDesc.vertex = vertex_state;
    renderPipelineDesc.fragment = &fragment_state;
    renderPipelineDesc.primitive = prim_state;
    renderPipelineDesc.multisample = multisample_state;

    pipeline = wgpuDeviceCreateRenderPipeline(*this->device, &renderPipelineDesc);
  }

  void RasterizationRenderAPI::Terminate()
  {
    wgpuRenderPipelineRelease(pipeline);
  }
};