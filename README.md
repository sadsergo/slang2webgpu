# Slang2WebGPU

## Build
Clone this repo with all its submodules:

  * git clone https://github.com/sadsergo/slang2webgpu.git
  * git submodule update --init --recursive

### Install packages

  * sudo apt install -y cmake build-essential libxrandr-dev libxinerama-dev
  * sudo apt install -y libxcursor-dev mesa-common-dev libx11-xcb-dev pkg-config nodejs npm
  * install Slang binaries: https://github.com/shader-slang/slang/releases/tag/v2025.10.2
  * Move Slang binaries into external folder as external/slang/files

### Linux

  * cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  * cmake --build build -j16
