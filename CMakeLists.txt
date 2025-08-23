cmake_minimum_required(VERSION 3.27.8)
project("Bootanical Gardens")

set(CMAKE_CXX_STANDARD 26)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

set(CPM_SOURCE_CACHE external)
include(cmake/CPM.cmake)
CPMAddPackage("gh:charles-lunarg/vk-bootstrap#v1.4.312")
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.10")
CPMAddPackage("gh:libsdl-org/SDL_ttf#release-3.2.2")
CPMAddPackage("gh:libsdl-org/SDL_image#release-3.2.4")
CPMAddPackage("gh:Neargye/magic_enum#v0.9.6")
set(FASTGLTF_DOWNLOAD_SIMDJSON ON)
set(FASTGLTF_ENABLE_TESTS OFF)
set(FASTGLTF_ENABLE_EXAMPLES OFF)
set(FASTGLTF_ENABLE_DOCS OFF)
CPMAddPackage("gh:spnda/fastgltf#v0.8.0")
CPMAddPackage("gh:mattreecebentley/plf_colony#ce7a8ab6e8a2923407f614291635a1ec0e3648f8")  # v7.5.26
CPMAddPackage(
        NAME glm
        GIT_TAG 1.0.1
        GITHUB_REPOSITORY g-truc/glm
        OPTIONS CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON  # Workaround for glm bug #1279
)
CPMAddPackage("gh:ibireme/yyjson#0.10.0")
CPMAddPackage("gh:jeremy-rifkin/cpptrace#v0.7.5")
CPMAddPackage("gh:SpartanJ/efsw#1.4.0")

add_executable(BootanicalGardens
        src/Component.cpp
        src/Entity.cpp
        src/Game/Components/Plant.cpp
        src/Game/Components/PlayerController.cpp
        src/Game/Game.cpp
        src/Game/LevelParser.cpp
        src/InputEngine/Input.cpp
        src/RenderEngine/CommandBuffer.cpp
        src/RenderEngine/DescriptorSetAllocator.cpp
        src/RenderEngine/DescriptorSetRequirer.cpp
        src/RenderEngine/Framebuffer.cpp
        src/RenderEngine/GraphicsDevice.cpp
        src/RenderEngine/GraphicsInstance.cpp
        src/RenderEngine/Pipeline.cpp
        src/RenderEngine/RenderGraph.cpp
        src/RenderEngine/RenderPass/OpaqueRenderPass.cpp
        src/RenderEngine/RenderPass/RenderPass.cpp
        src/RenderEngine/Renderable/Material.cpp
        src/RenderEngine/Renderable/Mesh.cpp
        src/RenderEngine/Renderable/Renderable.cpp
        src/RenderEngine/Renderable/Texture.cpp
        src/RenderEngine/Renderable/Vertex.cpp
        src/RenderEngine/Resources/Buffer.cpp
        src/RenderEngine/Resources/Image.cpp
        src/RenderEngine/Resources/Resource.cpp
        src/RenderEngine/Resources/StagingBuffer.cpp
        src/RenderEngine/Resources/UniformBuffer.cpp
        src/RenderEngine/Shader.cpp
        src/RenderEngine/Window.cpp

        src/Tools/ClassName.h
        src/Tools/StringHash.h

        src/Filesystem/UpdateListener.cpp
        src/Filesystem/UpdateListener.hpp
)
target_sources(BootanicalGardens PRIVATE main.cpp)
target_compile_definitions(BootanicalGardens PRIVATE VK_NO_PROTOTYPES)
target_include_directories(BootanicalGardens PRIVATE ${CMAKE_SOURCE_DIR} ${Vulkan_INCLUDE_DIRS} ${vk-bootstrap_SOURCE_DIR}/src ${SDL_SOURCE_DIR}/include ${SDL_ttf_SOURCE_DIR}/include ${SDL_image_SOURCE_DIR}/include ${magic_enum_SOURCE_DIR}/include ${SPIRV-Reflect_SOURCE_DIR} ${plf_colony_SOURCE_DIR})
target_link_libraries(BootanicalGardens PRIVATE Vulkan::shaderc_combined vk-bootstrap::vk-bootstrap SDL3::SDL3-shared SDL3_ttf::SDL3_ttf-shared SDL3_image::SDL3_image-shared glm::glm yyjson fastgltf cpptrace::cpptrace efsw)
target_link_options(BootanicalGardens PRIVATE -Wl,-rpath=$ORIGIN)

# Move all .dlls to the exe
if (WIN32)
    add_custom_command(TARGET BootanicalGardens POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:BootanicalGardens> $<TARGET_RUNTIME_DLLS:BootanicalGardens>
            COMMAND_EXPAND_LISTS
    )
endif ()