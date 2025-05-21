#include "GraphicsInstance.hpp"

#include "Window.hpp"

#include <cpptrace/cpptrace.hpp>
#include <SDL3/SDL_vulkan.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

thread_local GraphicsInstance::DebugData GraphicsInstance::debugData{};
vkb::Instance GraphicsInstance::instance{};

VkBool32 GraphicsInstance::debugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, const VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
  if (messageTypes == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    std::cerr << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }
  std::string message = pCallbackData->pMessage;
  size_t pos = message.find('\n');
  while (pos != std::string::npos) {
    message.replace(pos, 1, "\n\t\t");
    pos = message.find('\n', pos + 3);
  }
  std::string stacktrace = cpptrace::generate_trace().to_string(true);
  stacktrace.reserve(std::ranges::count(stacktrace, '\n') * 2 + stacktrace.length());
  pos = stacktrace.find('\n');
  while (pos != std::string::npos) {
    stacktrace.replace(pos, 1, "\n\t\t");
    pos = stacktrace.find('\n', pos + 3);
  }
  std::cerr << "[Vulkan Debug Callback]\n\t" << message << "\n\t" << stacktrace << std::endl;
  if (pUserData != nullptr) {
    const auto* debugData = static_cast<DebugData*>(pUserData);
#ifndef NDEBUG
    if (debugData->command != nullptr) {
      stacktrace = debugData->command->stacktrace.to_string(true);
      stacktrace.reserve(std::ranges::count(stacktrace, '\n') * 2 + stacktrace.length());
      pos = stacktrace.find('\n');
      while (pos != std::string::npos) {
        stacktrace.replace(pos, 1, "\n\t\t");
        pos = stacktrace.find('\n', pos + 3);
      }
      std::cerr << "\t" << "Caused by Command " << std::hex << debugData->command << " recorded at:\n\t" << stacktrace << std::endl;
    }
#endif
  }
  return VK_FALSE;
}

void GraphicsInstance::create(const std::vector<const char*>& extensions) {
  if (const VkResult result = volkInitialize(); result != VK_SUCCESS) showError(result, "Failed to load Vulkan.");
  SDL_DestroyWindow(Window::initialize());
  uint32_t extensionCount;
  char const* const* SDLExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
  vkb::InstanceBuilder builder;
  builder.enable_extensions(extensionCount, SDLExtensions);
  builder.enable_extensions(extensions);
  builder.set_app_name("Example Application").set_app_version(0, 0, 1);
  builder.set_engine_name("Example Engine").set_engine_version(0, 0, 1);
#ifndef NDEBUG
  builder.enable_validation_layers();
  builder.set_debug_callback(&GraphicsInstance::debugCallback);
  builder.set_debug_callback_user_data_pointer(&GraphicsInstance::debugData);
#else
  builder.enable_validation_layers(false);
#endif
  vkb::Result<vkb::Instance> buildResult = builder.build();
  if (!buildResult.has_value()) showError(buildResult.vk_result(), "Failed to create the Vulkan instance");
  instance = buildResult.value();
  volkLoadInstanceOnly(instance.instance);
}

void GraphicsInstance::destroy() {
  destroy_instance(instance);
  instance = {};
  volkFinalize();
}

void GraphicsInstance::setDebugDataCommand(CommandBuffer::Command* command) { debugData.command = command; }