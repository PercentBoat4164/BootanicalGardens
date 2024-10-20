#pragma once

#include "GraphicsInstance.hpp"
#include "RenderGraph.hpp"

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

public:
  RenderGraph renderer;

  static SDL_Window* initialize();
  static void cleanupInitialization(SDL_Window* window);

  explicit Window(const GraphicsDevice& device);
  ~Window();

  void draw();
};
