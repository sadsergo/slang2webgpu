cmake_minimum_required(VERSION 3.20)
project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Enable debug symbols, minimal optimization, AddressSanitizer, and strong warnings
    add_compile_options(
        -g
        -O1
        -fno-omit-frame-pointer
        -Wall
        -Wextra
        -Wshadow
        -Wpointer-arith
        -Wcast-align
        -Wformat=2
        -Wnull-dereference
    )
    add_link_options(
        -fno-omit-frame-pointer
    )
    # Optionally, enable Undefined Behavior Sanitizer as well:
    # add_compile_options(-fsanitize=undefined)
    # add_link_options(-fsanitize=undefined)
endif()

# add_subdirectory(external/webgpu)
# include_directories(external/webgpu/include)

# add_library(wgpu_native SHARED IMPORTED)
# set_target_properties(wgpu_native PROPERTIES
#   IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/webgpu_native/"
# )

include_directories(${CMAKE_SOURCE_DIR}/external/webgpu_native/include)
link_directories(${CMAKE_SOURCE_DIR}/external/webgpu_native/lib)

find_package(slang REQUIRED PATHS external/slang/cmake NO_DEFAULT_PATH)
include_directories(external/slang/include)

find_package(glfw3 REQUIRED)

include_directories(App/ Image/)

add_executable(app main.cpp App/app.cpp)

# WebGPU window application
target_link_libraries(app PRIVATE wgpu_native slang::slang glfw)

# Example with Slang and WebGPU
# add_executable(slang2webgpu_example examples/example.cpp)
# target_link_libraries(slang2webgpu_example PRIVATE webgpu slang::slang glfw)
# target_copy_webgpu_binaries(slang2webgpu_example)