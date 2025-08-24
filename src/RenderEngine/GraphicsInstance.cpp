#include "GraphicsInstance.hpp"

#include "Window.hpp"

#include <cpptrace/cpptrace.hpp>
#include <SDL3/SDL_vulkan.h>
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#if !NDEBUG
  /**@todo: Write a better debug log function once logging methods have been decided upon.*/
  #define VMA_DEBUG_LOG_FORMAT(format, ...)  \
  do {                                       \
    printf((format), __VA_ARGS__);           \
    printf("\n");                            \
  } while(false)
#endif
#include <vma/vk_mem_alloc.h>

#if BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS & !VK_EXT_debug_utils
#pragma message("This project is configured to use the Vulkan debug utils, but the VK_EXT_debug_utils extension is not available. Usage of the Vulkan debug utils will be disabled.")
#undef BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
#endif

thread_local GraphicsInstance::DebugData GraphicsInstance::debugData{};
vkb::Instance GraphicsInstance::instance{};
std::unordered_set<uint32_t> GraphicsInstance::enabledExtensions{};

// ReSharper disable once CppDFAConstantFunctionResult
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
  std::cerr << "[Vulkan Debug Callback]\n\t" << message << "\n\t" << stacktrace << "\n";
  if (pUserData != nullptr) {
    const auto* debugData = static_cast<DebugData*>(pUserData);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
    if (debugData->command != nullptr) {
      stacktrace = debugData->command->trace.resolve().to_string(true);
      stacktrace.reserve(std::ranges::count(stacktrace, '\n') * 2 + stacktrace.length());
      pos = stacktrace.find('\n');
      while (pos != std::string::npos) {
        stacktrace.replace(pos, 1, "\n\t\t");
        pos = stacktrace.find('\n', pos + 3);
      }
      std::cerr << "\t" << "Caused by Command " << std::hex << debugData->command << " recorded at:\n\t" << stacktrace << "\n";
    }
#endif
  }
  return VK_FALSE;
}

void GraphicsInstance::create(std::vector<const char*> extensions) {
  enabledExtensions.clear();
  if (const VkResult result = volkInitialize(); result != VK_SUCCESS) showError(result, "Failed to load Vulkan.");
  SDL_DestroyWindow(Window::initialize());
  uint32_t SDLExtensionCount;
  char const* const* SDLExtensions = SDL_Vulkan_GetInstanceExtensions(&SDLExtensionCount);
  const std::remove_cvref_t<decltype(extensions)>::size_type extensionCount = extensions.size();
  extensions.resize(extensionCount + SDLExtensionCount);
  for (; SDLExtensionCount > 0; --SDLExtensionCount) extensions[extensionCount + SDLExtensionCount - 1] = SDLExtensions[SDLExtensionCount - 1];
  for (const char* extension: extensions) enabledExtensions.emplace(Tools::hash(extension));
  vkb::InstanceBuilder builder;
  builder.enable_extensions(SDLExtensionCount, SDLExtensions);
  builder.enable_extensions(extensions);
  builder.set_app_name("Bootanical Gardens").set_app_version(0, 0, 1);
  builder.set_engine_name("Boo Engine").set_engine_version(0, 0, 1);
#if !NDEBUG & defined(BOOTANICAL_GARDENS_ENABLE_VULKAN_VALIDATION)
  builder.enable_validation_layers(std::getenv(BOOTANICAL_GARDENS_ENABLE_VULKAN_VALIDATION) != nullptr);
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

bool GraphicsInstance::extensionEnabled(uint32_t nameHash) {
  return enabledExtensions.contains(nameHash);
}

void GraphicsInstance::setDebugDataCommand(CommandBuffer::Command* command) { debugData.command = command; }

void GraphicsInstance::destroy() {
  destroy_instance(instance);
  instance = {};
  volkFinalize();
}

void GraphicsInstance::showError(const std::string& message) {
  if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr)) std::cerr << "Error: " << message << std::endl;
}

void GraphicsInstance::showSDLError() {
  showError(SDL_GetError());
}