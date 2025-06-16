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

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#define UNUSED(x) (void)(x)
#define WEBGPU_STR(str) WGPUStringView{ str, sizeof(str) - 1 }

namespace WGPU
{
void error_callback(int error, const char* description);

class Application
{
public:
  // Init everything and return true if all went right
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

private:
//  Get next texture view from swapchain
WGPUTextureView getNextSurfaceViewData();

//  Load buffer data and renderPass to commandBuffer
void onGui(WGPURenderPassEncoder renderPass);

// Init ImGui
void initImGui();

// Terminate ImGui
void terminateImGui(); 

// private:
public:
GLFWwindow* window;
WGPUInstance instance;
WGPUSurface surface;
WGPUAdapter adapter;
WGPUDevice device;
WGPUQueue queue;
WGPUSurfaceCapabilities surface_capabilities;
WGPUTexture frame_texture;
WGPUTextureView frame_texture_view;
};
};