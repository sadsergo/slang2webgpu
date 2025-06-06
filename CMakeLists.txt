cmake_minimum_required(VERSION 3.20)
project(app CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/webgpu)
include_directories(external/webgpu/include)

find_package(slang REQUIRED PATHS external/slang/cmake NO_DEFAULT_PATH)

include_directories(external/slang/include)

add_executable(app main.cpp)

target_link_libraries(app PRIVATE webgpu slang::slang)
target_copy_webgpu_binaries(app)