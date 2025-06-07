#include "app.h"

namespace WGPU
{
void error_callback(int error, const char* description)
{
  UNUSED(error);

  fprintf(stderr, "Error: %s\n", description);
}

// Utility function to request an adapter synchronously
static void handle_request_adapter(WGPURequestAdapterStatus status,
                                   WGPUAdapter adapter, WGPUStringView message,
                                   void *userdata1, void *userdata2)
{
  UNUSED(status);
  UNUSED(message);
  UNUSED(userdata2);

  *(WGPUAdapter *)userdata1 = adapter;
}

static void handle_request_device(WGPURequestDeviceStatus status,
                                  WGPUDevice device, WGPUStringView message,
                                  void *userdata1, void *userdata2)
{
  UNUSED(status);
  UNUSED(message);
  UNUSED(userdata2);

  *(WGPUDevice *)userdata1 = device;
}

void onDeviceError(WGPUErrorType type, const char* message, void*) 
{
  UNUSED(type);

  std::cerr << "Device error: " << message << std::endl;
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
  instance = wgpuCreateInstance(&desc);

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

  WGPURequestAdapterOptions adapterOpts = {.nextInChain = nullptr, .compatibleSurface = surface};

  wgpuInstanceRequestAdapter(instance, 
    &adapterOpts,
    (const WGPURequestAdapterCallbackInfo) {
      .callback = handle_request_adapter,
      .userdata1 = &adapter
    });

  wgpuAdapterRequestDevice(adapter, NULL, (const WGPURequestDeviceCallbackInfo) {
    .callback = handle_request_device,
    .userdata1 = &device
  });

  queue = wgpuDeviceGetQueue(device);

  surface_capabilities = {0};
  wgpuSurfaceGetCapabilities(surface, adapter, &surface_capabilities);

  WGPUSurfaceConfiguration config = {};

  config.device = device;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.format = surface_capabilities.formats[0],
  config.presentMode = WGPUPresentMode_Fifo;
  config.nextInChain = nullptr;
  config.viewFormatCount = 0;
  config.viewFormats = nullptr;
  config.alphaMode = WGPUCompositeAlphaMode_Auto;

  int width, height;
  glfwGetWindowSize(window, &width, &height);
  config.width = width;
  config.height = height;

  wgpuSurfaceConfigure(surface, &config);

  //? Release the adapter only after it has been fully utilized
	//? wgpuAdapterRelease(adapter);

  return true;
}

bool Application::IsRunning() const
{
  return !glfwWindowShouldClose(window);
}

WGPUTextureView Application::getNextSurfaceViewData()
{
  //  Get the surface texture
  WGPUSurfaceTexture surfaceTexture;

  wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
	if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Error) {
		return nullptr;
	}

  // Create a view for this surface texture
	WGPUTextureViewDescriptor viewDescriptor;
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
  viewDescriptor.label = WEBGPU_STR("Surface texture view");
  viewDescriptor.usage = WGPUTextureUsage_RenderAttachment;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

  // wgpuTextureRelease(surfaceTexture.texture);

  return targetView;
}

void Application::mainLoop()
{
  glfwPollEvents();

  WGPUTextureView targetView = getNextSurfaceViewData();
  if (!targetView)
  {
    return;
  }

  //  Create a command encoder for the draw call
  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

  // Create the render pass that clears the screen with our color
  WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;

  // The attachment part of the render pass descriptor describes the target texture of the pass
	WGPURenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

  renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

  renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

  // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

  // Finally encode and submit the render pass
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder);

  std::cout << "Submitting command..." << std::endl;
	wgpuQueueSubmit(queue, 1, &command);
	wgpuCommandBufferRelease(command);
	std::cout << "Command submitted." << std::endl;

	// At the end of the frame
	wgpuTextureViewRelease(targetView);

  wgpuSurfacePresent(surface);
  wgpuDevicePoll(device, false, nullptr);
}

void Application::Terminate()
{
  wgpuSurfaceCapabilitiesFreeMembers(surface_capabilities);
  wgpuSurfaceUnconfigure(surface);
  wgpuSurfaceRelease(surface);
  wgpuQueueRelease(queue);
  wgpuDeviceRelease(device);
  wgpuAdapterRelease(adapter);
  glfwDestroyWindow(window);
  wgpuInstanceRelease(instance);
  glfwTerminate();
}
};