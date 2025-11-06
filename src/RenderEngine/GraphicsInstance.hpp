#pragma once
#include "magic_enum/magic_enum.hpp"

#include <SDL3/SDL_messagebox.h>
#include <VkBootstrap.h>

#include <unordered_set>

class GraphicsDevice;

class GraphicsInstance {
  static thread_local struct DebugData {
    void* command;
  } debugData;

public:
  static vkb::Instance instance;
  static std::unordered_set<uint32_t> enabledExtensions;

  static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
  
  static void create(std::vector<const char*> extensions);
  /**
   * Check for availability of instance extensions. This function <em>must</em> be called before using any functionality provided by an instance extension.
   * <c>if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) { // code that names objects }</c>
   * @param nameHash The hash of the extension's name.
   * @return <c>true</c> if the extension is enabled and <c>false</c> if not.
   */
  static bool extensionEnabled(uint64_t nameHash);
  static void setDebugDataCommand(void* command);
  static void destroy();

  static void showError(const std::string& message);
  template<typename T> requires std::is_enum_v<T> static void showError(const T result, const std::string& message) { showError("Error: " + message + " | " + std::string(magic_enum::enum_name<T>(result))); }
  static void showSDLError();
};
