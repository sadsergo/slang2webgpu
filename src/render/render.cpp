#include "render.h"
#include "utils.h"
#include "mesh.h"
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
    renderPassColorAttachment.view = multisample_texture_view;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.resolveTarget = frame_texture_view;
    renderPassColorAttachment.clearValue = WGPUColor{ 0.0, 0.0, 0.0, 0.0 };
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

    WGPURenderPassDepthStencilAttachment depthStencilAttachment {};
    depthStencilAttachment.view = depth_texture_view;
    depthStencilAttachment.depthClearValue = 1.0f;
    depthStencilAttachment.depthLoadOp = WGPULoadOp_Clear;
    depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.depthReadOnly = (WGPUBool)false;
    depthStencilAttachment.stencilClearValue = 0;
    depthStencilAttachment.stencilLoadOp = WGPULoadOp_Clear;
		depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
    depthStencilAttachment.stencilReadOnly = true;

    WGPURenderPassDescriptor renderPassDesc{};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
    renderPassDesc.timestampWrites = nullptr;

    WGPURenderPassEncoder render_pass_encoder = wgpuCommandEncoderBeginRenderPass(command_encoder, &renderPassDesc);

    wgpuRenderPassEncoderSetPipeline(render_pass_encoder, pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass_encoder, 0, vertex_buffer, 0, wgpuBufferGetSize(vertex_buffer));
    wgpuRenderPassEncoderSetBindGroup(render_pass_encoder, 0, bind_group, 0, nullptr);
    wgpuRenderPassEncoderDraw(render_pass_encoder, indices_count, 1, 0, 0);

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
    dest.buffer = output_buffer;
    dest.layout.bytesPerRow = bytesPerRow;
    dest.layout.offset = 0;
    dest.layout.rowsPerImage = HEIGHT;

    wgpuCommandEncoderCopyTextureToBuffer(command_encoder, &src, &dest, &textureSize);

    WGPUCommandBufferDescriptor cmd_desc{};
    cmd_desc.label = { "Rasterization command buffer", WGPU_STRLEN };
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, &cmd_desc);

    wgpuQueueSubmit(*queue, 1, &command_buffer);
    wgpuCommandBufferRelease(command_buffer);
    wgpuCommandEncoderRelease(command_encoder);

    wgpuDevicePoll(*device, false, nullptr);
  }

  void RasterizationRenderAPI::Init(std::shared_ptr<WGPUDevice> device, std::shared_ptr<WGPUQueue> queue, const int indices_count, WGPUBuffer output_buffer, WGPUBuffer vertex_buffer, WGPUBuffer index_buffer, WGPUBuffer uniform_buffer)
  {
    this->device = device;
    this->queue = queue;
    this->output_buffer = output_buffer;
    this->vertex_buffer = vertex_buffer;
    this->index_buffer = index_buffer;
    this->uniform_buffer = uniform_buffer;
    this->indices_count = indices_count;

    //  Init texture and its view
    //  Create texture
    WGPUExtent3D textureSize = {(uint32_t)WIDTH, (uint32_t)HEIGHT, 1};

    WGPUTextureDescriptor multisampleTextureDesc{};
    multisampleTextureDesc.dimension = WGPUTextureDimension_2D;
    multisampleTextureDesc.format = WGPUTextureFormat_RGBA8Unorm;
    multisampleTextureDesc.size = textureSize;
    multisampleTextureDesc.sampleCount = 4;
    multisampleTextureDesc.viewFormatCount = 0;
    multisampleTextureDesc.viewFormats = nullptr;
    multisampleTextureDesc.mipLevelCount = 1;
    multisampleTextureDesc.label = {"Rasterization multisample texture", WGPU_STRLEN};
    multisampleTextureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst | WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;

    multisample_texture = wgpuDeviceCreateTexture(*device, &multisampleTextureDesc);

    WGPUTextureViewDescriptor multisampleTextureViewDesc {};
    multisampleTextureViewDesc.aspect = WGPUTextureAspect_All;
    multisampleTextureViewDesc.baseArrayLayer = 0;
    multisampleTextureViewDesc.arrayLayerCount = 1;
    multisampleTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
    multisampleTextureViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
    multisampleTextureViewDesc.mipLevelCount = 1;
    multisampleTextureViewDesc.baseMipLevel = 0;
    multisampleTextureViewDesc.label = {"Rasterization multisample texture view", WGPU_STRLEN};
    
    multisample_texture_view = wgpuTextureCreateView(multisample_texture, &multisampleTextureViewDesc);

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

    WGPUBlendState blend_state = utils::wgpu_create_blend_state(true);

     /* Depth stencil state */
    WGPUDepthStencilState depth_stencil_state;
    utils::set_default_depth_stencil_state(depth_stencil_state);

    WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(*this->device, &tmp3);

    std::vector<WGPUVertexAttribute> vertexAttribs(4);

    // pos
    vertexAttribs[0].shaderLocation = 0;
    vertexAttribs[0].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[0].offset = 0;

    // normal
    vertexAttribs[1].shaderLocation = 1;
    vertexAttribs[1].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[1].offset = offsetof(Vertex, normal);

    // color
    vertexAttribs[2].shaderLocation = 2;
    vertexAttribs[2].format = WGPUVertexFormat_Float32x3;
    vertexAttribs[2].offset = offsetof(Vertex, color);

    // texture coords
    vertexAttribs[3].shaderLocation = 3;
    vertexAttribs[3].format = WGPUVertexFormat_Float32x2;
    vertexAttribs[3].offset = offsetof(Vertex, texCoord);

    WGPUVertexBufferLayout vertexBufferLayout;
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttribs.size());
    vertexBufferLayout.attributes = vertexAttribs.data();
    vertexBufferLayout.arrayStride = sizeof(Vertex);
    vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

    WGPUBindGroupLayoutEntry bindingLayout {};
    bindingLayout.binding = 0;
    bindingLayout.visibility = WGPUShaderStage_Fragment | WGPUShaderStage_Vertex;
    bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(Uniforms);

    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &bindingLayout;
    
    WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(*device, &bindGroupLayoutDesc);

    WGPUPipelineLayoutDescriptor layoutDesc {};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.label = {"Rasterization pipeline layout", WGPU_STRLEN};
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)(&bindGroupLayout);
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(*device, &layoutDesc);

    WGPUVertexState vertex_state = { .module = shader_module, .entryPoint = {"vs_main", WGPU_STRLEN}, .constantCount = 0, .constants = nullptr };
    vertex_state.bufferCount = 1;
    vertex_state.buffers = &vertexBufferLayout;
    
    const WGPUColorTargetState tmp4 = {
      .format = WGPUTextureFormat_RGBA8Unorm,
      .blend = &blend_state,
      .writeMask = WGPUColorWriteMask_All,
    };
    const WGPUColorTargetState targets[] = { tmp4 };
    const WGPUFragmentState fragment_state = { .module = shader_module, .entryPoint = {"fs_main", WGPU_STRLEN}, .constantCount = 0, .constants = nullptr, .targetCount = 1, .targets = targets };

    // Create the depth texture
    WGPUTextureDescriptor depthTextureDesc {};
    depthTextureDesc.dimension = WGPUTextureDimension_2D;
    depthTextureDesc.format = depth_stencil_state.format;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.size = {WIDTH, HEIGHT, 1};
    depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
    depthTextureDesc.sampleCount = 4;
    depthTextureDesc.viewFormatCount = 1;
    depthTextureDesc.viewFormats = (WGPUTextureFormat*)(&depth_stencil_state.format);

    WGPUTexture depthTexture = wgpuDeviceCreateTexture(*device, &depthTextureDesc);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depthTextureViewDesc {};
    depthTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
    depthTextureViewDesc.baseArrayLayer = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.baseMipLevel = 0;
    depthTextureViewDesc.mipLevelCount = 1;
    depthTextureViewDesc.dimension = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.format = depth_stencil_state.format;

    depth_texture_view = wgpuTextureCreateView(depthTexture, &depthTextureViewDesc);

    WGPUBindGroupEntry binding {};
    binding.binding = 0;
    binding.buffer = uniform_buffer;
    binding.offset = 0;
    binding.size = sizeof(Uniforms);

    WGPUBindGroupDescriptor bindGroupDesc {};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = bindGroupLayoutDesc.entryCount;
    bindGroupDesc.entries = &binding;
    
    bind_group = wgpuDeviceCreateBindGroup(*device, &bindGroupDesc);

    // MSAA
    const WGPUPrimitiveState prim_state = { .topology = WGPUPrimitiveTopology_TriangleList, .stripIndexFormat = WGPUIndexFormat_Undefined, .frontFace = WGPUFrontFace_CCW, .cullMode = WGPUCullMode_None };
    const WGPUMultisampleState multisample_state = { .count = 4, .mask = 0xFFFFFFFF, .alphaToCoverageEnabled = false };

    WGPURenderPipelineDescriptor renderPipelineDesc{};
    renderPipelineDesc.label = {"Rasterization pipeline", WGPU_STRLEN};
    renderPipelineDesc.layout = layout;
    renderPipelineDesc.vertex = vertex_state;
    renderPipelineDesc.fragment = &fragment_state;
    renderPipelineDesc.primitive = prim_state;
    renderPipelineDesc.multisample = multisample_state;
    renderPipelineDesc.depthStencil = &depth_stencil_state;

    pipeline = wgpuDeviceCreateRenderPipeline(*this->device, &renderPipelineDesc);
  }

  void RasterizationRenderAPI::Terminate()
  {
    wgpuRenderPipelineRelease(pipeline);
    
    wgpuTextureViewRelease(frame_texture_view);
    wgpuTextureRelease(frame_texture);
    wgpuTextureViewRelease(depth_texture_view);
  }
};