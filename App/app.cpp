#include "app.h"

namespace WGPU
{
void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

// Utility function to request an adapter synchronously
static void handle_request_adapter(WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter, WGPUStringView message,
                                   void *userdata1, void *userdata2)
{
  *(WGPUAdapter *)userdata1 = adapter;
}

static void handle_request_device(WGPURequestDeviceStatus status,
                                  WGPUDevice device, WGPUStringView message,
                                  void *userdata1, void *userdata2)
{
  *(WGPUDevice *)userdata1 = device;
}

void onDeviceError(WGPUErrorType type, const char* message, void*) {
    std::cerr << "Device error: " << message << std::endl;
}

// WGPUSurface Application::createSurface(WGPUInstance instance)
// {
//   Display* x11_display = glfwGetX11Display();
//   Window x11_window = glfwGetX11Window(window);

//   WGPUSurfaceDescriptorFromXlibWindow fromXlibWindow;
//   fromXlibWindow.chain.next = NULL;
//   fromXlibWindow.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
//   fromXlibWindow.display = x11_display;
//   fromXlibWindow.window = x11_window;

//   WGPUSurfaceDescriptor surfaceDescriptor;
//   surfaceDescriptor.nextInChain = &fromXlibWindow.chain;
//   surfaceDescriptor.label = NULL;

//   return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
// }

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

  Display *x11_display = glfwGetX11Display();
  Window x11_window = glfwGetX11Window(window);

  const WGPUSurfaceSourceXlibWindow xlibWindowDesc = {
    .chain =
        (const WGPUChainedStruct){
            .sType = WGPUSType_SurfaceSourceXlibWindow,
        },
    .display = x11_display,
    .window = x11_window,
  };

  const WGPUSurfaceDescriptor surfaceDesc = {
    .nextInChain = (const WGPUChainedStruct *)&xlibWindowDesc
  }; 

  surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);

  WGPURequestAdapterOptions adapterOpts = {.compatibleSurface = surface};
  adapterOpts.nextInChain = nullptr;

  WGPUAdapter adapter = NULL;
  wgpuInstanceRequestAdapter(instance, 
    &adapterOpts,
    (const WGPURequestAdapterCallbackInfo) {
      .callback = handle_request_adapter,
      .userdata1 = &adapter
    });

  device = NULL;
  wgpuAdapterRequestDevice(adapter, NULL, (const WGPURequestDeviceCallbackInfo) {
    .callback = handle_request_device,
    .userdata1 = &device
  });

  queue = wgpuDeviceGetQueue(device);

  // WGPUSurfaceConfiguration config = {};
  // config.nextInChain = nullptr;



  // wgpuSurfaceConfigure(surface, &config);

  // surface = glfwGetWGPUSurface(instance, window);

  // WGPUSurfaceConfiguration config = {};
  // WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);


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
  wgpuSurfaceUnconfigure(surface);
  wgpuSurfaceRelease(surface);
  glfwDestroyWindow(window);
  glfwTerminate();
}
};