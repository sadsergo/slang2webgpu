#pragma once

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

#include <cassert>
#include <iostream>

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
  bool IsRunning();

private:
WGPUSurface createSurface(WGPUInstance instance);
GLFWwindow* window;
WGPUInstance instance;
WGPUSurface surface;
WGPUAdapter adapter;
WGPUDevice device;
WGPUQueue queue;
WGPUSurfaceCapabilities surface_capabilities;
};
};