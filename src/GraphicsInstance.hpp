#pragma once
#include <VkBootstrap.h>

#include <chrono>
#include <SDL3/SDL_messagebox.h>
#include <magic_enum/magic_enum.hpp>

class GraphicsDevice;

class GraphicsInstance {

public:
  static vkb::Instance instance;

  static void create(const std::vector<const char*>& extensions);
  static void destroy();
  static void showError(VkResult result, const std::string& message);

  template<typename T> static void showError(const vkb::Result<T>& error, const std::string& message) {
    if (error.has_value()) return;
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", (message + " | " + error.error().message() + " | " + std::string(magic_enum::enum_name(error.vk_result()))).c_str(), nullptr);
  }

  template<typename T> requires (std::is_convertible_v<T, bool>) static void showError(const T& error, const std::string& message) {
    if (error) return;
    (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
  }
};
