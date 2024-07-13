#include "GraphicsInstance.hpp"

#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#include "Window.hpp"

#include <volk.h>

#include <SDL3/SDL_vulkan.h>

vkb::Instance GraphicsInstance::instance{};

void GraphicsInstance::create(const std::vector<const char*>& extensions) {
  showError(volkInitialize(), "Failed to load Vulkan.");
  Window::initialize();
  uint32_t extensionCount;
  std::vector<const char*> SDLExtensions;
  char const* const* const pSDLExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
  SDLExtensions.assign(extensionCount, *pSDLExtensions);
  vkb::InstanceBuilder builder;
  builder.enable_extensions(SDLExtensions);
  builder.enable_extensions(extensions);
  builder.set_app_name("Example Application").set_app_version(0, 0, 1);
  builder.set_engine_name("Example Engine").set_engine_version(0, 0, 1);
#ifndef NDEBUG
  builder.enable_validation_layers();
  builder.use_default_debug_messenger();
#else
  builder.enable_validation_layers(false);
#endif
  vkb::Result<vkb::Instance> buildResult = builder.build();
  showError(buildResult, "Failed to create the Vulkan instance.");
  instance = buildResult.value();
  volkLoadInstanceOnly(instance.instance);
}

void GraphicsInstance::destroy() {
  vkb::destroy_instance(instance);
  instance = {};
  volkFinalize();
}

void GraphicsInstance::showError(const VkResult result, const std::string& message) {
  if (result == VK_SUCCESS) return;
  (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", (message + " | " + std::string(magic_enum::enum_name(result))).c_str(), nullptr);
}