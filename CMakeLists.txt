cmake_minimum_required(VERSION 3.20)
project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/webgpu)
include_directories(external/webgpu/include)

find_package(slang REQUIRED PATHS external/slang/cmake NO_DEFAULT_PATH)
include_directories(external/slang/include)

find_package(glfw3 REQUIRED)

include_directories(App/)

add_subdirectory(external/glfw3webgpu)
include_directories(external/glfw3webgpu)
# set_target_properties(glfw3webgpu PROPERTIES LINKER_LANGUAGE C)

add_executable(app main.cpp App/app.cpp)
add_executable(slang2webgpu_example examples/example.cpp)

# WebGPU window application
target_link_libraries(app PRIVATE webgpu slang::slang glfw glfw3webgpu)
target_copy_webgpu_binaries(app)

# Example with Slang and WebGPU
target_link_libraries(slang2webgpu_example PRIVATE webgpu slang::slang glfw)
target_copy_webgpu_binaries(slang2webgpu_example)