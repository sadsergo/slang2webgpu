#include <iostream>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <slang.h>
#include <slang-com-ptr.h>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

#include <GLFW/glfw3.h>

#include "app.h"

int main()
{
  WGPU::Application app;

  if (!app.Initialize())
  {
    return 1;
  }

  //  Load image
  // app.image2Texture("data/example.png");

  uint8_t* data = nullptr;
  int width = 0, height = 0, channels = 0;

  app.loadImage("data/example.png", &data, width, height, channels);

  uint32_t bytesPerRowUnpadded = width * 4;
  uint32_t bytesPerRow = bytesPerRowUnpadded;
  uint32_t bufferSize = bytesPerRow * height;

  std::vector<uint8_t> aligned_data(bufferSize, 0);

  for (int i = 0; i < height; i++)
  {
    memcpy(aligned_data.data() + i * bytesPerRow, data + i * bytesPerRowUnpadded, bytesPerRowUnpadded);
  }

  //  Create buffer
  WGPUBufferDescriptor textureBufferDesc{};
  textureBufferDesc.size = bufferSize;
  textureBufferDesc.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
  textureBufferDesc.label = WEBGPU_STR("frame texture buffer");

  app.output_buffer = wgpuDeviceCreateBuffer(app.device, &textureBufferDesc);
  wgpuQueueWriteBuffer(app.queue, app.output_buffer, 0, aligned_data.data(), textureBufferDesc.size);

  //  Create texture
  WGPUExtent3D textureSize = {(uint32_t)width, (uint32_t)height, 1};

  WGPUTextureDescriptor textureDesc{};
  textureDesc.dimension = WGPUTextureDimension_2D;
  textureDesc.format = WGPUTextureFormat_RGBA8Unorm;
  textureDesc.size = textureSize;
  textureDesc.sampleCount = 1;
  textureDesc.viewFormatCount = 0;
  textureDesc.viewFormats = nullptr;
  textureDesc.mipLevelCount = 1;
  textureDesc.label = WEBGPU_STR("Input");
  textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

  app.frame_texture = wgpuDeviceCreateTexture(app.device, &textureDesc);

  WGPUTextureViewDescriptor textureViewDesc{};
  textureViewDesc.aspect = WGPUTextureAspect_All;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;
  textureViewDesc.dimension = WGPUTextureViewDimension_2D;
  textureViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.label = WEBGPU_STR("Input");
  
  app.frame_texture_view = wgpuTextureCreateView(app.frame_texture, &textureViewDesc);

  while (app.IsRunning())
  {
    app.mainLoop();
  }

  app.Terminate();

  return 0;
}