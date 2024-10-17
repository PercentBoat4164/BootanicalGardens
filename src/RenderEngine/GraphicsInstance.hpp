#pragma once
#include <VkBootstrap.h>

#include <magic_enum/magic_enum.hpp>

#include <iostream>

class GraphicsDevice;

class GraphicsInstance {

public:
  static vkb::Instance instance;

  static void create(const std::vector<const char*>& extensions);
  static void destroy();
  static void showError(VkResult result, const std::string& message);

  template<typename T> static void showError(const vkb::Result<T>& error, const std::string& message) {
    if (!error.has_value()) showError(error.vk_result(), message);
  }

  template<typename T> requires (std::is_convertible_v<T, bool>) static void showError(const T& error, const std::string& message) {
    if (!error) std::cout << "Error: " << message << std::endl;
  }
};
