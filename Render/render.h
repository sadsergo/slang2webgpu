#pragma once

#include <cstdint>
#include <memory>
#include <LiteMath.h>

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

using LiteMath::float3;

namespace WGPU
{ 
class RenderAPI
{
public:
  RenderAPI(const uint32_t RENDER_WIDTH, const uint32_t RENDER_HEIGHT) : WIDTH(RENDER_WIDTH), HEIGHT(RENDER_HEIGHT) {}

  virtual void draw(uint8_t* data) const;
  virtual void init(std::shared_ptr<WGPUDevice> device, std::shared_ptr<WGPUQueue> queue, std::shared_ptr<WGPUTexture> frame_texture, std::shared_ptr<WGPUTextureView> frame_texture_view); 

private:
  uint32_t WIDTH, HEIGHT;

  std::shared_ptr<WGPUDevice> device;
  std::shared_ptr<WGPUQueue> queue;
  std::shared_ptr<WGPUTexture> frame_texture;
  std::shared_ptr<WGPUTextureView> frame_texture_view;
};

class RasterizationRenderAPI : virtual public RenderAPI
{
public:
  RasterizationRenderAPI(const uint32_t RENDER_WIDTH, const uint32_t RENDER_HEIGHT) : RenderAPI(RENDER_WIDTH, RENDER_HEIGHT) {}
  
  void draw(uint8_t* data) const override;
  void init(std::shared_ptr<WGPUDevice> device, std::shared_ptr<WGPUQueue> queue, std::shared_ptr<WGPUTexture> frame_texture, std::shared_ptr<WGPUTextureView> frame_texture_view) override;

public:
};

};