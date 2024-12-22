cmake_minimum_required(VERSION 3.27.8)
project("Bootanical Gardens")

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

set(CPM_SOURCE_CACHE external)
include(cmake/CPM.cmake)
CPMAddPackage("gh:charles-lunarg/vk-bootstrap#v${Vulkan_VERSION}")
#@todo: Update SDL to version 3 when SDL_ttf has tags for a compatible version
CPMAddPackage("gh:libsdl-org/SDL#release-2.30.10")
CPMAddPackage("gh:libsdl-org/SDL_ttf#release-2.22.0")
CPMAddPackage("gh:libsdl-org/SDL_image#release-2.8.2")
CPMAddPackage("gh:Neargye/magic_enum#v0.9.6")
CPMAddPackage("gh:KhronosGroup/SPIRV-Reflect#vulkan-sdk-${Vulkan_VERSION}.0")
CPMAddPackage("gh:zeux/volk#vulkan-sdk-${Vulkan_VERSION}.0")
set(FASTGLTF_DOWNLOAD_SIMDJSON ON)
set(FASTGLTF_ENABLE_TESTS OFF)
set(FASTGLTF_ENABLE_EXAMPLES OFF)
set(FASTGLTF_ENABLE_DOCS OFF)
CPMAddPackage("gh:spnda/fastgltf#v0.8.0")
CPMAddPackage("gh:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator#v3.1.0")
CPMAddPackage(
        NAME glm
        GIT_TAG 1.0.1
        GITHUB_REPOSITORY g-truc/glm
        OPTIONS CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON # Workaround for glm bug #1279
)
CPMAddPackage("gh:ibireme/yyjson#0.10.0")
CPMAddPackage("gh:SpartanJ/efsw#1.4.0")

add_executable(BootanicalGardens
        src/Component.cpp
        src/Entity.cpp
        src/Game/Components/Plant.cpp
        src/Game/Components/PlayerController.cpp
        src/Game/Game.cpp
        src/Game/LevelParser.cpp
        src/InputEngine/Input.cpp
        src/RenderEngine/Buffer.cpp
        src/RenderEngine/CommandBuffer.cpp
        src/RenderEngine/DescriptorAllocator.cpp
        src/RenderEngine/GraphicsDevice.cpp
        src/RenderEngine/GraphicsInstance.cpp
        src/RenderEngine/Image.cpp
        src/RenderEngine/Pipeline.cpp
        src/RenderEngine/RenderPass.cpp
        src/RenderEngine/Renderable/Material.cpp
        src/RenderEngine/Renderable/Mesh.cpp
        src/RenderEngine/Renderable/Renderable.cpp
        src/RenderEngine/Renderable/Texture.cpp
        src/RenderEngine/Renderable/Vertex.cpp
        src/RenderEngine/Renderer.cpp
        src/RenderEngine/RenderGraph.cpp
        src/RenderEngine/Resource.cpp
        src/RenderEngine/Shader.cpp
        src/RenderEngine/ShaderUsage.cpp
        src/RenderEngine/Window.cpp

        src/Tools/ClassName.h
        src/Tools/StringHash.h

        ${SPIRV-Reflect_SOURCE_DIR}/spirv_reflect.c
        src/Filesystem/UpdateListener.cpp
        src/Filesystem/UpdateListener.hpp
)
target_sources(BootanicalGardens PRIVATE main.cpp)
target_compile_definitions(BootanicalGardens PRIVATE VK_NO_PROTOTYPES)
target_include_directories(BootanicalGardens PRIVATE ${CMAKE_SOURCE_DIR} ${Vulkan_INCLUDE_DIRS} ${vk-bootstrap_SOURCE_DIR}/src ${SDL_SOURCE_DIR}/include ${SDL_ttf_SOURCE_DIR}/include ${SDL_image_SOURCE_DIR} ${magic_enum_SOURCE_DIR}/include ${SPIRV-Reflect_SOURCE_DIR})
target_link_libraries(BootanicalGardens PRIVATE volk::volk_headers vk-bootstrap::vk-bootstrap SDL2 SDL2_ttf SDL2_image GPUOpen::VulkanMemoryAllocator Vulkan::shaderc_combined glm::glm yyjson fastgltf efsw)
target_link_options(BootanicalGardens PRIVATE -Wl,-rpath=$ORIGIN)

# Move all .dlls to the exe
if (WIN32)
    add_custom_command(TARGET BootanicalGardens POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:BootanicalGardens> $<TARGET_RUNTIME_DLLS:BootanicalGardens>
            COMMAND_EXPAND_LISTS
    )
endif ()