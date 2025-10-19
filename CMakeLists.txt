cmake_minimum_required(VERSION 3.27.8)
project("Bootanical Gardens")

set(CMAKE_CXX_STANDARD 26)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)
find_package(OpenEXR REQUIRED)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

set(CPM_SOURCE_CACHE external)
include(cmake/CPM.cmake)
CPMAddPackage("gh:charles-lunarg/vk-bootstrap@1.4.312")
CPMAddPackage("gh:libsdl-org/SDL#release-3.2.24")
CPMAddPackage("gh:Neargye/magic_enum#v0.9.7")
set(FASTGLTF_DOWNLOAD_SIMDJSON ON)
set(FASTGLTF_ENABLE_TESTS OFF)
set(FASTGLTF_ENABLE_EXAMPLES OFF)
set(FASTGLTF_ENABLE_DOCS OFF)
CPMAddPackage("gh:spnda/fastgltf@0.9.0")
set(DRACO_INSTALL OFF)
CPMAddPackage("gh:google/draco#1.5.7")
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
        src/RenderEngine/RenderPass/CollectShadowsRenderPass.cpp
        src/RenderEngine/RenderPass/GBufferRenderPass.cpp
        src/RenderEngine/RenderPass/RenderPass.cpp
        src/RenderEngine/RenderPass/ShadowRenderPass.cpp
        src/RenderEngine/MeshGroup/Material.cpp
        src/RenderEngine/MeshGroup/Mesh.cpp
        src/RenderEngine/MeshGroup/MeshGroup.cpp
        src/RenderEngine/MeshGroup/Texture.cpp
        src/RenderEngine/MeshGroup/Vertex.cpp
        src/RenderEngine/Resources/Buffer.cpp
        src/RenderEngine/Resources/Image.cpp
        src/RenderEngine/Resources/Resource.cpp
        src/RenderEngine/Resources/StagingBuffer.cpp
        src/RenderEngine/Resources/UniformBuffer.cpp
        src/RenderEngine/Shader.cpp
        src/RenderEngine/Window.cpp

        src/Tools/ClassName.h

        src/Filesystem/UpdateListener.cpp
)
target_sources(BootanicalGardens PRIVATE main.cpp)
target_compile_definitions(BootanicalGardens PRIVATE VK_NO_PROTOTYPES)
target_include_directories(BootanicalGardens PRIVATE ${CMAKE_SOURCE_DIR} ${Vulkan_INCLUDE_DIRS} ${vk-bootstrap_SOURCE_DIR}/src ${SDL_SOURCE_DIR}/include ${openexr_SOURCE_DIR}/include ${magic_enum_SOURCE_DIR}/include ${draco_SOURCE_DIR}/src ${CMAKE_BINARY_DIR} ${SPIRV-Reflect_SOURCE_DIR} ${plf_colony_SOURCE_DIR})
target_link_libraries(BootanicalGardens PUBLIC Vulkan::shaderc_combined vk-bootstrap::vk-bootstrap SDL3::SDL3-shared glm::glm OpenEXR::OpenEXR yyjson fastgltf draco::draco cpptrace::cpptrace efsw)
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    target_compile_definitions(BootanicalGardens PRIVATE
            BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING                                           # provides a full stacktrace for each command added to a command buffer.
            BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS                                               # provides names for Vulkan objects in graphics debuggers.
            BOOTANICAL_GARDENS_ENABLE_VULKAN_VALIDATION="BOOTANICAL_GARDENS_ENABLE_VULKAN_VALIDATION"  # enables vulkan validation if an environment variable matching this string is present
            BOOTANICAL_GARDENS_ENABLE_READABLE_SHADER_VARIABLE_NAMES                                   # provides human readable names for each shader variable.
    )
endif ()
# Move all .dlls to the exe
if (WIN32)
    add_custom_command(TARGET BootanicalGardens POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:BootanicalGardens> $<TARGET_RUNTIME_DLLS:BootanicalGardens>
            COMMAND_EXPAND_LISTS
    )
endif ()