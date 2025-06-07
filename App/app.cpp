#include "app.h"

namespace WGPU
{
void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

bool Application::Initialize()
{
  if (!glfwInit())
  {
    std::cerr << "NO GLFW INIT\n";
    return false;
  }

  glfwSetErrorCallback(error_callback);
  
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(640, 480, "WebGPU app", nullptr, nullptr);

  if (!window) {
    std::cerr << "Could not open window!" << std::endl;
    glfwTerminate();
    return false;
  }

  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  // Create the instance using this descriptor
  WGPUInstance instance = wgpuCreateInstance(&desc);

  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return false;
  }

  std::cout << "WGPU instance: " << instance << std::endl;

  surface = glfwGetWGPUSurface(instance, window);

  return true;
}

bool Application::IsRunning()
{
  return !glfwWindowShouldClose(window);
}

void Application::mainLoop()
{
  glfwPollEvents();
}

void Application::Terminate()
{
  // wgpuSurfaceRelease(surface);
  glfwDestroyWindow(window);
  glfwTerminate();
}
};