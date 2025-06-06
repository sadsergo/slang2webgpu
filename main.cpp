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

void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* pUserData) {
    UserData* userData = reinterpret_cast<UserData*>(pUserData);
    if (status == WGPURequestAdapterStatus_Success) {
        userData->adapter = adapter;
    } else {
        std::cout << "Could not get WebGPU adapter: " << message << std::endl;
    }
    userData->requestEnded = true;
}

WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
    UserData userData;
    wgpuInstanceRequestAdapter(
        instance,
        options,
        onAdapterRequestEnded,
        &userData
    );
    // Wait until the callback sets requestEnded to true
    while (!userData.requestEnded) {
        // You may want to yield or sleep a bit here in real code
    }
    assert(userData.adapter != nullptr && "Failed to acquire adapter");
    return userData.adapter;
}

WGPUDevice requestDeviceSync(WGPUAdapter adapter, const WGPUDeviceDescriptor* descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    } userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* pUserData) {
        UserData* userData = reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData->device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData->requestEnded = true;
    };

    wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, &userData);

    while (!userData.requestEnded) {
        // Optionally sleep/yield here
    }
    assert(userData.device != nullptr && "Failed to acquire device");
    return userData.device;
}

void onDeviceError(WGPUErrorType type, const char* message, void*) {
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
  
  // IComponentType* program = nullptr;
  // compileRequest->getProgram(&program);

  // ProgramLayout* programLayout = program->getLayout();
}

void execute_shader()
{
  // The vector size
  WGPUInstanceDescriptor desc = {};
  desc.nextInChain = nullptr;

  // We create the instance using this descriptor
  WGPUInstance instance = wgpuCreateInstance(&desc);

  if (!instance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return;
  }

  std::cout << "WGPU instance: " << instance << std::endl;

  WGPURequestAdapterOptions adapterOpts = {};
  adapterOpts.nextInChain = nullptr;

  WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);
  std::cout << "WGPU adapter: " << adapter << std::endl;

  //  Create device
  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.nextInChain = nullptr;
  deviceDesc.label = "Cur Device"; // Optional: for debugging
  deviceDesc.requiredFeaturesCount = 0; // No special features
  deviceDesc.requiredFeatures = nullptr;
  deviceDesc.requiredLimits = nullptr;
  deviceDesc.defaultQueue.nextInChain = nullptr;
  deviceDesc.defaultQueue.label = "The default queue";

  WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);
  std::cout << "WGPU device: " << device << std::endl;

  wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr);

  //  Prepare data
  const size_t N = 1024;
  std::vector<float> a(N, 1.0f), b(N, 2.0f), result(N, 0);

  WGPUBufferDescriptor bufDesc = {};
  bufDesc.size = N * sizeof(float);
  bufDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;

  WGPUBuffer aBuffer = wgpuDeviceCreateBuffer(device, &bufDesc);
  WGPUBuffer bBuffer = wgpuDeviceCreateBuffer(device, &bufDesc);
  WGPUBuffer resultBuffer = wgpuDeviceCreateBuffer(device, &bufDesc);

  wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), aBuffer, 0, a.data(), bufDesc.size);
  wgpuQueueWriteBuffer(wgpuDeviceGetQueue(device), bBuffer, 0, b.data(), bufDesc.size);

  //  Create shade module
  auto shaderSrc = readFile("shaders/vector_add.wgsl");
  WGPUShaderModuleWGSLDescriptor wgslDesc = {};
  wgslDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
  wgslDesc.code = shaderSrc.c_str();
  WGPUShaderModuleDescriptor shaderDesc = {};
  shaderDesc.nextInChain = &wgslDesc.chain;
  WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

  // 5. Create compute pipeline
  WGPUComputePipelineDescriptor pipelineDesc = {};
  pipelineDesc.compute.module = shaderModule;
  pipelineDesc.compute.entryPoint = "computeMain";
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
  WGPUBuffer readBuffer = wgpuDeviceCreateBuffer(device, &readDesc);
  wgpuCommandEncoderCopyBufferToBuffer(encoder, resultBuffer, 0, readBuffer, 0, bufDesc.size);

  // 9. Submit
  WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
  wgpuQueueSubmit(wgpuDeviceGetQueue(device), 1, &cmd);

  // 10. Map and read back result
  struct Context {
    bool ready;
    WGPUBuffer buffer;
  };
  struct MapContext { bool done = false; WGPUBuffer buffer; };
  Context context = { false, readBuffer };

  auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* pUserData) {
    Context* context = reinterpret_cast<Context*>(pUserData);
    context->ready = true;
    std::cout << "Result buffer mapped with status " << status << std::endl;
    if (status != WGPUBufferMapAsyncStatus_Success) return;
  };

  wgpuBufferMapAsync(readBuffer, WGPUMapMode_Read, 0, bufDesc.size, onBuffer2Mapped, (void*)&context);

  while (!context.ready) {
    wgpuDevicePoll(device, false, nullptr);
}

  const float* mapped = static_cast<const float*>(wgpuBufferGetMappedRange(readBuffer, 0, bufDesc.size));
  std::memcpy(result.data(), mapped, bufDesc.size);
  wgpuBufferUnmap(readBuffer);

  // 11. Print first 10 results
  std::cout << "First 10 elements: ";
  for (size_t i = 0; i < 10; ++i) std::cout << result[i] << " ";
  std::cout << std::endl;
}

int main()
{
  gen_shader();
  execute_shader();

  return 0;
}