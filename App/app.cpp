#include "app.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

#define UNUSED(x) (void)(x)
#define WEBGPU_STR(str) WGPUStringView{ str, sizeof(str) - 1 }

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
  window = glfwCreateWindow(APP_WIDTH, APP_HEIGHT, "WebGPU app", nullptr, nullptr);

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

  #if defined(GLFW_EXPOSE_NATIVE_X11)

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

  #elif defined(GLFW_EXPOSE_NATIVE_WIN32)

  HWND hwnd = glfwGetWin32Window(window);
  HINSTANCE hinstance = GetModuleHandle(NULL);
  
  const WGPUChainedStruct tmp3 = {
    .sType = WGPUSType_SurfaceSourceWindowsHWND,
  };

  const WGPUSurfaceSourceWindowsHWND tmp2 = {
    .chain = tmp3,
    .hinstance = hinstance,
    .hwnd = hwnd,
  };

  const WGPUSurfaceDescriptor tmp1 = {
    .nextInChain = (const WGPUChainedStruct *)&tmp2,
  };

  surface = wgpuInstanceCreateSurface(instance, &tmp1);

  #endif

  WGPURequestAdapterOptions adapterOpts = {.nextInChain = nullptr, .compatibleSurface = surface};

  const WGPURequestAdapterCallbackInfo adapterCallbackInfo = {
    .callback = handle_request_adapter,
    .userdata1 = &adapter
  };

  device = std::make_shared<WGPUDevice>();

  const WGPURequestDeviceCallbackInfo deviceCallbackInfo = {
    .callback = handle_request_device,
    .userdata1 = device.get()
  };

  wgpuInstanceRequestAdapter(instance, &adapterOpts, adapterCallbackInfo);
  wgpuAdapterRequestDevice(adapter, NULL, deviceCallbackInfo);

  queue = std::make_shared<WGPUQueue>(wgpuDeviceGetQueue(*device));

  WGPUSurfaceConfiguration config = {};

  config.device = *device;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.format = WGPUTextureFormat_RGBA8Unorm,
  config.presentMode = WGPUPresentMode_Fifo; 
  config.nextInChain = nullptr;
  config.viewFormatCount = 0;
  config.viewFormats = nullptr;
  config.alphaMode = WGPUCompositeAlphaMode_Auto;

  // int width, height;
  // glfwGetWindowSize(window, &width, &height);
  config.width = APP_WIDTH;
  config.height = APP_HEIGHT;

  wgpuSurfaceConfigure(surface, &config);

  initImGui();
  initFrameBuffers();

  // Release the adapter only after it has been fully utilized
	wgpuAdapterRelease(adapter);

  return true;
}

void Application::initFrameBuffers()
{
  int width, height;
  glfwGetWindowSize(window, &width, &height);

  uint32_t bytesPerRowUnpadded = width * 4;
  uint32_t bytesPerRow = bytesPerRowUnpadded;
  uint32_t bufferSize = bytesPerRow * height;

  //  Create buffer
  WGPUBufferDescriptor textureBufferDesc{};
  textureBufferDesc.size = bufferSize;
  textureBufferDesc.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
  textureBufferDesc.label = WEBGPU_STR("frame texture buffer");

  output_buffer = wgpuDeviceCreateBuffer(*device, &textureBufferDesc);
  
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

  frame_texture = std::make_shared<WGPUTexture>(wgpuDeviceCreateTexture(*device, &textureDesc));

  WGPUTextureViewDescriptor textureViewDesc {};
  textureViewDesc.aspect = WGPUTextureAspect_All;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;
  textureViewDesc.dimension = WGPUTextureViewDimension_2D;
  textureViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.label = WEBGPU_STR("Input");
  
  frame_texture_view = std::make_shared<WGPUTextureView>(wgpuTextureCreateView(*frame_texture, &textureViewDesc));
}

void Application::image2Texture(const std::string& path)
{
  int width = 0, height = 0, channels = 0;
  uint8_t* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

  if (data == nullptr) throw std::runtime_error("Could not load input texture!");

  WGPUExtent3D textureSize = {(uint32_t)width, (uint32_t)height, 1};

  //  Create texture
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
  
  frame_texture = std::make_shared<WGPUTexture>(wgpuDeviceCreateTexture(*device, &textureDesc));

  WGPUTexelCopyTextureInfo dest{};
  dest.texture = *frame_texture;
  dest.origin = {0, 0, 0};
  dest.aspect = WGPUTextureAspect_All;
  dest.mipLevel = 0;

  WGPUTexelCopyBufferLayout source{};
  source.offset = 0;
  source.bytesPerRow = channels * sizeof(uint8_t) * width;
  source.rowsPerImage = height;

  wgpuQueueWriteTexture(*queue, &dest, data, (size_t)(channels * width * height * sizeof(uint8_t)), &source, &textureSize);

  WGPUTextureViewDescriptor textureViewDesc{};
  textureViewDesc.aspect = WGPUTextureAspect_All;
  textureViewDesc.baseArrayLayer = 0;
  textureViewDesc.arrayLayerCount = 1;
  textureViewDesc.dimension = WGPUTextureViewDimension_2D;
  textureViewDesc.format = WGPUTextureFormat_RGBA8Unorm;
  textureViewDesc.mipLevelCount = 1;
  textureViewDesc.baseMipLevel = 0;
  textureViewDesc.label = WEBGPU_STR("Input");
  
  frame_texture_view = std::make_shared<WGPUTextureView>(wgpuTextureCreateView(*frame_texture, &textureViewDesc));

  stbi_image_free(data);
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

  return targetView;
}

void Application::userInput()
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }
}

void Application::initImGui()
{
  // Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

  //  Setup Platform/Renderer backends
  ImGui_ImplWGPU_InitInfo imGuiRenderInfo{};
  imGuiRenderInfo.Device = *device;
  imGuiRenderInfo.NumFramesInFlight = 3;
  imGuiRenderInfo.RenderTargetFormat = WGPUTextureFormat_RGBA8Unorm;
  imGuiRenderInfo.DepthStencilFormat = WGPUTextureFormat_Undefined;

  ImGui_ImplGlfw_InitForOther(window, true);
  ImGui_ImplWGPU_Init(&imGuiRenderInfo);
}

void Application::terminateImGui()
{
  ImGui_ImplWGPU_Shutdown();
	ImGui_ImplGlfw_Shutdown();
}

void Application::onGui(WGPURenderPassEncoder renderPass)
{
  ImGui_ImplWGPU_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  //  Display image
  {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    drawList->AddImage((ImTextureID)(*frame_texture_view), {0, 0}, {APP_WIDTH, APP_HEIGHT});
  }

  ImGui::SetNextWindowSize(ImVec2(350, 50));
  ImGui::Begin("Performance");
  ImGuiIO& io = ImGui::GetIO();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.f / io.Framerate, io.Framerate);
  ImGui::End();

  ImGui::Render();
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), renderPass);
}

bool Application::loadImage(const std::string& path, uint8_t** data, int &width, int &height, int &channels)
{
  *data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  if (data == nullptr) return false;

  return true;
}

void Application::copyOutBuffer2FrameTexture(WGPUCommandEncoder encoder)
{
  uint32_t bytesPerRowUnpadded = APP_WIDTH * 4;
  uint32_t bytesPerRow = bytesPerRowUnpadded;
  uint32_t bufferSize = bytesPerRow * APP_HEIGHT;

  WGPUExtent3D textureSize = {(uint32_t)APP_WIDTH, (uint32_t)APP_HEIGHT, 1};

  //  Copy buffer to texture
  WGPUTexelCopyTextureInfo dest{};
  dest.texture = *frame_texture;
  dest.origin = {0, 0, 0};
  dest.aspect = WGPUTextureAspect_All;
  dest.mipLevel = 0;

  WGPUTexelCopyBufferInfo source{};
  source.buffer = output_buffer;
  source.layout.bytesPerRow = bytesPerRow;
  source.layout.offset = 0;
  source.layout.rowsPerImage = APP_HEIGHT;

  //  Copy output buffer to texture for further drawing
  wgpuCommandEncoderCopyBufferToTexture(encoder, &source, &dest, &textureSize);
}

void Application::mainLoop()
{
  //  Process all pending events
  glfwPollEvents();
  userInput();

  WGPUTextureView targetView = getNextSurfaceViewData();
  
  if (!targetView)
  {
    return;
  }

  //  Create a command encoder for the draw call
  WGPUCommandEncoderDescriptor encoderDesc = {};
  encoderDesc.nextInChain = nullptr;
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(*device, &encoderDesc);

  copyOutBuffer2FrameTexture(encoder);

  // Create the render pass that clears the screen with our color
  WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;

  // The attachment part of the render pass descriptor describes the target texture of the pass
	WGPURenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 1.0, 1.0, 1.0, 1.0 };

  renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

  renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

  // Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

  onGui(renderPass);

	wgpuRenderPassEncoderEnd(renderPass);
	wgpuRenderPassEncoderRelease(renderPass);

  // Finally encode and submit the render pass
	WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
	wgpuCommandEncoderRelease(encoder);

	wgpuQueueSubmit(*queue, 1, &command);
	wgpuCommandBufferRelease(command);

	// At the end of the frame
	wgpuTextureViewRelease(targetView);

  wgpuSurfacePresent(surface);
  wgpuDevicePoll(*device, false, nullptr);
}

void Application::terminateBuffers()
{
  wgpuBufferRelease(output_buffer);
  wgpuTextureViewRelease(*frame_texture_view);
  wgpuTextureRelease(*frame_texture);
}

void Application::Terminate()
{
  render_api->Terminate();

  terminateBuffers();
  
  wgpuSurfaceUnconfigure(surface);
  wgpuSurfaceRelease(surface);
  
  wgpuQueueRelease(*queue);
  wgpuDeviceRelease(*device);
  
  terminateImGui();
  
  glfwDestroyWindow(window);
  wgpuInstanceRelease(instance);
  
  glfwTerminate();
}
};