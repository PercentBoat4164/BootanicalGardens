cmake_minimum_required(VERSION 3.27.8)
project(GarDEADner)

set(CMAKE_CXX_STANDARD 26)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

set(CPM_SOURCE_CACHE external)
include(cmake/CPM.cmake)
CPMAddPackage("gh:charles-lunarg/vk-bootstrap#v${Vulkan_VERSION}")
CPMAddPackage("gh:libsdl-org/SDL#main")
CPMAddPackage("gh:libsdl-org/SDL_ttf#main")
CPMAddPackage("gh:libsdl-org/SDL_image#main")
CPMAddPackage("gh:Neargye/magic_enum#master")
CPMAddPackage("gh:KhronosGroup/SPIRV-Reflect#vulkan-sdk-${Vulkan_VERSION}.0")
CPMAddPackage("gh:zeux/volk#vulkan-sdk-${Vulkan_VERSION}.0")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator#v3.1.0")

add_executable(GarDEADner
        src/DescriptorAllocator.cpp
        src/DescriptorAllocator.hpp
        src/GraphicsDevice.cpp
        src/GraphicsDevice.hpp
        src/GraphicsInstance.cpp
        src/GraphicsInstance.hpp
        src/Image.cpp
        src/Image.hpp
        src/Pipeline.cpp
        src/Pipeline.hpp
        src/RenderPass.cpp
        src/RenderPass.hpp
        src/Renderer.cpp
        src/Renderer.hpp
        src/Resource.cpp
        src/Resource.hpp
        src/Shader.cpp
        src/Shader.hpp
        src/ShaderUsage.cpp
        src/ShaderUsage.hpp
        src/Window.cpp
        src/Window.hpp

        ${SPIRV-Reflect_SOURCE_DIR}/spirv_reflect.c
)
target_sources(GarDEADner PRIVATE main.cpp)
target_compile_definitions(GarDEADner PRIVATE VK_NO_PROTOTYPES NO_SDL_VULKAN_TYPEDEFS)
target_include_directories(GarDEADner PRIVATE ${CMAKE_SOURCE_DIR} ${Vulkan_INCLUDE_DIRS} ${vk-bootstrap_SOURCE_DIR}/src ${SDL_SOURCE_DIR}/include ${SDL_ttf_SOURCE_DIR}/include ${SDL_image_SOURCE_DIR} ${magic_enum_SOURCE_DIR}/include ${SPIRV-Reflect_SOURCE_DIR})
target_link_libraries(GarDEADner PRIVATE volk::volk_headers vk-bootstrap::vk-bootstrap SDL3::SDL3-shared SDL3_ttf::SDL3_ttf-shared SDL3_image::SDL3_image-shared GPUOpen::VulkanMemoryAllocator Vulkan::shaderc_combined)
target_link_options(GarDEADner PRIVATE -Wl,-rpath=$ORIGIN)
