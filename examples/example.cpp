#include <iostream>
#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <slang.h>
#include <slang-com-ptr.h>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

using namespace slang;

// Utility to read file content into a string
std::string readFile(const char* path) {
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

struct UserData {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
};

#define UNUSED(x) (void)(x)
#define WEBGPU_STR(str) WGPUStringView{ str, sizeof(str) - 1 }

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

void gen_shader()
{
  // Read shader source
  std::string shaderSource = readFile("shaders/vector_add.slang");

  IGlobalSession* globalSession = nullptr;
  createGlobalSession(&globalSession);

  ISession* session = nullptr;
  SessionDesc sessionDesc;
  globalSession->createSession(sessionDesc, &session);

  ICompileRequest* compileRequest = nullptr;
  session->createCompileRequest(&compileRequest);

  compileRequest->addCodeGenTarget(SLANG_WGSL);

  int translationUnitIndex = compileRequest->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, nullptr);
  compileRequest->addTranslationUnitSourceString(translationUnitIndex, "shaders/vector_add.slang", shaderSource.c_str());

  int entryPointIndex = compileRequest->addEntryPoint(translationUnitIndex, "computeMain", SLANG_STAGE_COMPUTE);

  SlangResult result = compileRequest->compile();
  if (SLANG_FAILED(result)) {
      // Handle compilation errors
      const char* diagnostics = compileRequest->getDiagnosticOutput();
      // Process error messages
      std::cout << diagnostics << std::endl;
      return;
  }

  size_t size = 0;
  const char* code = (const char*)compileRequest->getEntryPointCode(entryPointIndex, &size);

  std::ofstream f;
  f.open("shaders/vector_add.wgsl");
  f << code;
  f.close();
}

void execute_shader()
{
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  // We create the instance using this descriptor
  WGPUInstance instance = wgpuCreateInstance(&desc);

  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return;
  }

  std::cout << "WGPU instance: " << instance << std::endl;

  WGPUAdapter adapter;
  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;

  const WGPURequestAdapterCallbackInfo adapterCallbackInfo = {
    .callback = handle_request_adapter,
    .userdata1 = &adapter
  };

  wgpuInstanceRequestAdapter(instance, &adapterOpts, adapterCallbackInfo);
  std::cout << "WGPU adapter: " << adapter << std::endl;

  WGPUDevice device;

  const WGPURequestDeviceCallbackInfo deviceCallbackInfo = {
    .callback = handle_request_device,
    .userdata1 = &device
  };

  wgpuAdapterRequestDevice(adapter, NULL, deviceCallbackInfo);
  std::cout << "WGPU device: " << device << std::endl;

  WGPUQueue queue = wgpuDeviceGetQueue(device);
  std::cout << "WGPU queue: " << queue << std::endl;

  wgpuAdapterRelease(adapter);

  //  Prepare data
  const size_t N = 1024;
  std::vector<float> a(N), b(N), result(N, 0);

  for (int i = 0; i < N; i++)
  {
    a[i] = 2;
    b[i] = 3 * i;
  }

  WGPUBufferDescriptor bufDesc = {};
  bufDesc.size = N * sizeof(float);
  bufDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;

  WGPUBuffer aBuffer = wgpuDeviceCreateBuffer(device, &bufDesc);
  WGPUBuffer bBuffer = wgpuDeviceCreateBuffer(device, &bufDesc);
  WGPUBuffer resultBuffer = wgpuDeviceCreateBuffer(device, &bufDesc);

  wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), aBuffer, 0, a.data(), bufDesc.size);
  wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), bBuffer, 0, b.data(), bufDesc.size);

  //  Load the shader module
  std::string shader_code = readFile("shaders/vector_add.wgsl");

  const WGPUChainedStruct tmp1 = { .sType = WGPUSType_ShaderSourceWGSL };
  
  const WGPUShaderSourceWGSL tmp2 = {
    .chain = tmp1,
    .code = {shader_code.c_str(), WGPU_STRLEN},
  };

  const WGPUShaderModuleDescriptor tmp3 = {
    .nextInChain = (const WGPUChainedStruct *)&tmp2,
    .label = {"Rasterization shader module", WGPU_STRLEN}
  };

  WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(device, &tmp3);

  // 5. Create compute pipeline
  WGPUComputePipelineDescriptor pipelineDesc = {};
  pipelineDesc.compute.module = shader_module;
  pipelineDesc.compute.entryPoint = {"computeMain", WGPU_STRLEN};
  WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(device, &pipelineDesc);

  // 6. Create bind group layout and bind group
  WGPUBindGroupLayout layout = wgpuComputePipelineGetBindGroupLayout(pipeline, 0);
  WGPUBindGroupEntry entries[3] = {
      { .binding = 0, .buffer = aBuffer, .offset = 0, .size = bufDesc.size },
      { .binding = 1, .buffer = bBuffer, .offset = 0, .size = bufDesc.size },
      { .binding = 2, .buffer = resultBuffer, .offset = 0, .size = bufDesc.size }
  };
  WGPUBindGroupDescriptor bgDesc = {};
  bgDesc.layout = layout;
  bgDesc.entryCount = 3;
  bgDesc.entries = entries;
  WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

  // 7. Encode commands
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
  WGPUComputePassDescriptor passDesc = {};
  WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(encoder, &passDesc);
  wgpuComputePassEncoderSetPipeline(pass, pipeline);
  wgpuComputePassEncoderSetBindGroup(pass, 0, bindGroup, 0, nullptr);
  wgpuComputePassEncoderDispatchWorkgroups(pass, (N + 63) / 64, 1, 1);
  wgpuComputePassEncoderEnd(pass);

  // 8. Copy result buffer to a readable buffer
  WGPUBufferDescriptor readDesc = {};
  readDesc.size = bufDesc.size;
  readDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
  readDesc.mappedAtCreation = false;

  WGPUBuffer readBuffer = wgpuDeviceCreateBuffer(device, &readDesc);
  wgpuCommandEncoderCopyBufferToBuffer(encoder, resultBuffer, 0, readBuffer, 0, bufDesc.size);

  // 9. Submit
  WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
  wgpuQueueSubmit(wgpuDeviceGetQueue(device), 1, &cmd);

  // 10. Map and read back result
  struct CallbackData { uint8_t done; } cb = {0};
  auto map_callback = [](WGPUMapAsyncStatus status, WGPUStringView message, void *userdata1, void *userdata2) {
    ((struct CallbackData*)userdata1)->done = 1;
  };
  WGPUBufferMapCallbackInfo map_callback_info;
  map_callback_info.callback = map_callback;
  map_callback_info.userdata1 = &cb;

  wgpuBufferMapAsync(readBuffer, WGPUMapMode_Read, 0, readDesc.size, map_callback_info);
  while (!cb.done) { wgpuDevicePoll(device, false, nullptr); }

  const float* mapped = static_cast<const float*>(wgpuBufferGetMappedRange(readBuffer, 0, bufDesc.size));
  std::memcpy(result.data(), mapped, bufDesc.size);
  wgpuBufferUnmap(readBuffer);

  // 11. Print first 10 results
  std::cout << "First 10 elements: ";
  for (size_t i = 0; i < 10; ++i) std::cout << result[i] << " ";
  std::cout << std::endl;

  wgpuBufferRelease(aBuffer);
  wgpuBufferRelease(bBuffer);
  wgpuBufferRelease(resultBuffer);

  wgpuQueueRelease(queue);
  wgpuDeviceRelease(device);
  wgpuInstanceRelease(instance);
}

int main()
{
//   gen_shader();
  execute_shader();

  return 0;
}