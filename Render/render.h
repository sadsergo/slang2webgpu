#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <fstream>

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

  virtual void Draw() const = 0;
  virtual void Init(std::shared_ptr<WGPUDevice> device, std::shared_ptr<WGPUQueue> queue, std::shared_ptr<WGPUBuffer> output_buffer) = 0; 
  virtual void Terminate() = 0;

protected:
  uint32_t WIDTH, HEIGHT;

  std::shared_ptr<WGPUDevice> device;
  std::shared_ptr<WGPUQueue> queue;
};

class RasterizationRenderAPI : virtual public RenderAPI
{
public:
  RasterizationRenderAPI(const uint32_t RENDER_WIDTH, const uint32_t RENDER_HEIGHT) : RenderAPI(RENDER_WIDTH, RENDER_HEIGHT) {}
  
  void Draw() const override;
  void Init(std::shared_ptr<WGPUDevice> device, std::shared_ptr<WGPUQueue> queue, std::shared_ptr<WGPUBuffer> output_buffer) override;
  // void SetScene(const std::vector<SimpleMesh>& meshes);
  void Terminate() override;
public:

private:
  WGPURenderPipeline pipeline;
  std::shared_ptr<WGPUBuffer> output_buffer;
  WGPUTexture frame_texture;
  WGPUTextureView frame_texture_view;

  // std::vector<SimpleMesh> meshes;
};

};