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

  app.loadScene("c:/Users/nikit/Desktop/meshes/bulldozer.obj");

  app.render_api = std::make_shared<WGPU::RasterizationRenderAPI>(APP_WIDTH, APP_HEIGHT);
  app.render_api->Init(app.device, app.queue, app.output_buffer);

  while (app.IsRunning())
  {
    app.mainLoop();
  }

  app.Terminate();

  return 0;
}