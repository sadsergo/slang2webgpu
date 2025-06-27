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

#include <GLFW/glfw3.h>

#include "app.h"

int main()
{
  WGPU::Application app;

  if (!app.Initialize())
  {
    return 1;
  }

  //  Load image
  // app.image2Texture("data/example.png");

  uint8_t* data = nullptr;
  int width = 0, height = 0, channels = 0;

  if (app.loadImage("data/example.png", &data, width, height, channels) == false)
  {
    std::cout << "Coud not load image\n";
    return 1;
  }

  app.render_api = std::make_shared<WGPU::RasterizationRenderAPI>(width, height);
  app.render_api->Init(app.device, app.queue, app.output_buffer);

  uint32_t bytesPerRowUnpadded = width * 4;
  uint32_t bytesPerRow = bytesPerRowUnpadded;
  uint32_t bufferSize = bytesPerRow * height;

  wgpuQueueWriteBuffer(*app.queue, *app.output_buffer, 0, data, bufferSize);

  while (app.IsRunning())
  {
    app.mainLoop();
  }

  app.Terminate();

  return 0;
}