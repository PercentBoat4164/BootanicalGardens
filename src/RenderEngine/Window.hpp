#pragma once

#include "GraphicsInstance.hpp"
#include "Renderer.hpp"

#include <VkBootstrap.h>

class Image;
struct SDL_Window;

class Window {
  SDL_Window* window{nullptr};
  VkSurfaceKHR surface{VK_NULL_HANDLE};
  vkb::Swapchain swapchain;
  uint32_t swapchainIndex{};
  std::vector<Image> swapchainImages{};

  const GraphicsDevice& device;

  uint32_t getImage(VkSemaphore swapchainSemaphore);
  void present(const std::vector<VkSemaphore>& waitSemaphores) const;

public:
  Renderer renderer;

  static void initialize();

  explicit Window(const GraphicsDevice& device);
  ~Window();

  void draw();
};
