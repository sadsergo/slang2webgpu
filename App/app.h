#pragma once

#include <GLFW/glfw3.h>

#if defined(UNIX)
#define GLFW_EXPOSE_NATIVE_X11
#else
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3native.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <cassert>
#include <iostream>
#include <vector>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include "render.h"

constexpr uint32_t APP_WIDTH = 1024;
constexpr uint32_t APP_HEIGHT = 1024;

namespace WGPU
{
void error_callback(int error, const char* description);

class Application
{
public:
  //  Init everything and return true if all went right
  bool Initialize();

  //  Clean all App resources
  void Terminate();

  //  Draw a frame and handle events
  void mainLoop();

  //  Return true as long as the main loop should keep on running 
  bool IsRunning() const;

  void image2Texture(const std::string& path);

  //  Load image
  bool loadImage(const std::string& path, uint8_t** data, int &width, int &height, int &channels);

  //  Load scene
  void loadScene(const std::string& path);

  //  Create vertex buffer
  void createVertexBuffer();

  //  Create index buffer
  void createIndexBuffer();

  //  Process every interacted added event
  void userInput();

  //  Init render API
  void initRenderAPI();

private:
//  Init frame buffer, texture and its view
void initFrameBuffers();

//  Get next texture view from swapchain
WGPUTextureView getNextSurfaceViewData();

//  Copy data from output buffer to frame texture
void copyOutBuffer2FrameTexture(WGPUCommandEncoder encoder);

// Init ImGui
void initImGui();

//  Load buffer data and renderPass to commandBuffer
void onGui(WGPURenderPassEncoder renderPass);

//  Terminate ImGui
void terminateImGui(); 

//  Terminate buffers
void terminateBuffers();

// private:
public:
GLFWwindow* window;
WGPUInstance instance;
WGPUSurface surface;
WGPUAdapter adapter;
std::shared_ptr<WGPUDevice> device;
std::shared_ptr<WGPUQueue> queue;
WGPUTexture frame_texture;
WGPUTextureView frame_texture_view;
std::shared_ptr<WGPUBuffer> output_buffer;

std::shared_ptr<RenderAPI> render_api;
// std::vector<SimpleMesh> meshes;
};
};