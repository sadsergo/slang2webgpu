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

private:
WGPUTextureView getNextSurfaceViewData();

private:
GLFWwindow* window;
WGPUInstance instance;
WGPUSurface surface;
WGPUAdapter adapter;
WGPUDevice device;
WGPUQueue queue;
WGPUSurfaceCapabilities surface_capabilities;
};
};