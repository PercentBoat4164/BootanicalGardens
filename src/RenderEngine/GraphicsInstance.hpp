#pragma once
#include "CommandBuffer.hpp"
#include "magic_enum/magic_enum.hpp"

#include <SDL3/SDL_messagebox.h>
#include <VkBootstrap.h>

#include <iostream>

class GraphicsDevice;

class GraphicsInstance {
  static thread_local struct DebugData {
    CommandBuffer::Command* command;
  } debugData;

public:
  static vkb::Instance instance;

  static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
  
  static void create(const std::vector<const char*>& extensions);
  static void destroy();
  static void setDebugDataCommand(CommandBuffer::Command* command);

  static void showError(const std::string& message) { if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr)) std::cerr << "Error: " << message << std::endl; }
  template<typename T> requires std::is_enum_v<T> static void showError(const T result, const std::string& message) { showError("Error: " + message + " | " + std::string(magic_enum::enum_name<T>(result))); }
  static void showSDLError() { showError(SDL_GetError()); }
};
