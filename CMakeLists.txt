cmake_minimum_required(VERSION 3.20)
project(app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(NOMINMAX)

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

include_directories(${CMAKE_SOURCE_DIR}/external/webgpu_native/include)
link_directories(${CMAKE_SOURCE_DIR}/external/webgpu_native/lib)

find_package(slang REQUIRED PATHS external/slang/cmake NO_DEFAULT_PATH)
include_directories(external/slang/include)

# include_directories(${CMAKE_SOURCE_DIR}/external/LiteMath/external)
include_directories(${CMAKE_SOURCE_DIR}/external/LiteMath)

set(CPP_FILES
    main.cpp 
    App/app.cpp
    Render/render.cpp
    external/LiteMath/Image2d.cpp
)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
set(IMGUI_SOURCES 
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

add_library(ImGui STATIC ${IMGUI_SOURCES})
target_include_directories(ImGui PUBLIC ${IMGUI_DIR})
target_compile_definitions(ImGui PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_WGPU)

include_directories(App/ Image/ Render/)

if (WIN32)
    if (MSVC)
        add_compile_options(/W4)
        endif()

        set(OS_LIBRARIES d3dcompiler ws2_32 userenv bcrypt ntdll opengl32 Propsys RuntimeObject)

        #   Set GLFW paths
        set(GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/glfw/include)
        set(GLFW_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/external/glfw/lib-vc2022)

        #   Include GlFW headers
        include_directories(${GLFW_INCLUDE_DIR})
else()
    find_package(glfw3 REQUIRED)
endif()

add_executable(app ${CPP_FILES})

if (WIN32)
target_link_libraries(app PRIVATE ${OS_LIBRARIES} ${GLFW_LIBRARY_DIR}/glfw3.lib ${CMAKE_SOURCE_DIR}/external/webgpu_native/lib/wgpu_native.lib slang::slang ImGui)
else()
target_link_libraries(app PRIVATE wgpu_native slang::slang glfw ImGui)
endif()