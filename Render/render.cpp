#include "render.h"
#include <iostream>

#define UNUSED(x) (void)(x)

namespace WGPU
{
  std::string readFile(const char* path) {
      std::ifstream file(path, std::ios::binary);
      return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  }

  void RasterizationRenderAPI::Draw() const
  {
    WGPUCommandEncoderDescriptor command_encoder_desc = { .label = {"Rasterization command encoder", WGPU_STRLEN} };
    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(*device, &command_encoder_desc);

    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = frame_texture_view;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{ 0.0, 0.0, 0.0, 0.0 };
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDescriptor renderPassDesc{};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(command_encoder, &renderPassDesc);

    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, pipeline);
    wgpuRenderPassEncoderDraw(render_pass_encoder, 3, 1, 0, 0);

    wgpuRenderPassEncoderEnd(render_pass_encoder);
    wgpuRenderPassEncoderRelease(render_pass_encoder);

    uint32_t bytesPerRowUnpadded = WIDTH * 4;
    uint32_t bytesPerRow = bytesPerRowUnpadded;
    uint32_t bufferSize = bytesPerRow * HEIGHT;

    WGPUExtent3D textureSize = {(uint32_t)WIDTH, (uint32_t)HEIGHT, 1};
    WGPUTexelCopyTextureInfo src{};
    src.texture = frame_texture;
    src.origin = {0, 0, 0};
    src.aspect = WGPUTextureAspect_All;
    src.mipLevel = 0;

    WGPUTexelCopyBufferInfo dest{};
    dest.buffer = *output_buffer;
    dest.layout.bytesPerRow = bytesPerRow;
    dest.layout.offset = 0;
    dest.layout.rowsPerImage = HEIGHT;

    wgpuCommandEncoderCopyTextureToBuffer(command_encoder, &src, &dest, &textureSize);

    WGPUCommandBufferDescriptor cmd_desc{};
    cmd_desc.label = { "Rasterization command buffer", WGPU_STRLEN };
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, &cmd_desc);

    wgpuQueueSubmit(*queue, 1, &command_buffer);
    wgpuCommandEncoderRelease(command_encoder);

    wgpuDevicePoll(*device, false, nullptr);
  }

  void RasterizationRenderAPI::Init(std::shared_ptr<WGPUDevice> device, std::shared_ptr<WGPUQueue> queue, std::shared_ptr<WGPUBuffer> output_buffer)
  {
    this->device = device;
    this->queue = queue;
    this->output_buffer = output_buffer;

    //  Init texture and its view
    //  Create texture
    WGPUExtent3D textureSize = {(uint32_t)WIDTH, (uint32_t)HEIGHT, 1};

    WGPUTextureDescriptor textureDesc{};
    textureDesc.dimension = WGPUTextureDimension_2D;
    textureDesc.format = WGPUTextureFormat_RGBA8Unorm;
    textureDesc.size = textureSize;
    textureDesc.sampleCount = 1;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    textureDesc.mipLevelCount = 1;
    textureDesc.label = {"Rasterization texture", WGPU_STRLEN};
    textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;

    frame_texture = wgpuDeviceCreateTexture(*device, &textureDesc);

    WGPUTextureViewDescriptor textureViewDesc {};
    textureViewDesc.aspect = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.dimension = WGPUTextureViewDimension_2D;
    textureViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
    textureViewDesc.mipLevelCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.label = {"Rasterization texture view", WGPU_STRLEN};
    
    frame_texture_view = wgpuTextureCreateView(frame_texture, &textureViewDesc);
    
    //  Load the shader module
    std::string shader_code = readFile("shaders/rasterization.wgsl");

    const WGPUChainedStruct tmp1 = { .sType = WGPUSType_ShaderSourceWGSL };
    
    const WGPUShaderSourceWGSL tmp2 = {
      .chain = tmp1,
      .code = {shader_code.c_str(), WGPU_STRLEN},
    };

    const WGPUShaderModuleDescriptor tmp3 = {
      .nextInChain = (const WGPUChainedStruct *)&tmp2,
      .label = {"Rasterization shader module", WGPU_STRLEN}
    };

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(*this->device, &tmp3);

    WGPUPipelineLayoutDescriptor pipelineLayoutDesc{};
    pipelineLayoutDesc.label = {"Rasterization pipeline layout", WGPU_STRLEN};

    WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(*device, &pipelineLayoutDesc);

    WGPUVertexState vertex_state = { .module = shader_module, .entryPoint = {"vs_main", WGPU_STRLEN} };
    const WGPUColorTargetState tmp4 = {
      .format = WGPUTextureFormat_RGBA8Unorm,
      .writeMask = WGPUColorWriteMask_All,
    };
    const WGPUColorTargetState targets[] = { tmp4 };
    const WGPUFragmentState fragment_state = { .module = shader_module, .entryPoint = {"fs_main", WGPU_STRLEN}, .targetCount = 1, .targets = targets };

    const WGPUPrimitiveState prim_state = { .topology = WGPUPrimitiveTopology_TriangleList };
    const WGPUMultisampleState multisample_state = { .count = 1, .mask = 0xFFFFFFFF };

    WGPURenderPipelineDescriptor renderPipelineDesc{};
    renderPipelineDesc.label = {"Rasterization pipeline", WGPU_STRLEN};
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
    
    wgpuTextureViewRelease(frame_texture_view);
    wgpuTextureRelease(frame_texture);
  }

  // void RasterizationRenderAPI::SetScene(const std::vector<SimpleMesh>& meshes)
  // {
  //   this->meshes.resize(meshes.size());
  //   std::copy(meshes.begin(), meshes.end(), this->meshes.begin());
  // }
};