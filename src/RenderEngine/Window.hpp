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
  std::vector<std::shared_ptr<Image>> swapchainImages;

  const std::shared_ptr<GraphicsDevice> device;

public:
  static SDL_Window* initialize();
  static void cleanupInitialization(SDL_Window* window);

  explicit Window(const std::shared_ptr<GraphicsDevice>& device);
  ~Window();

  [[nodiscard]] VkExtent3D getResolution() const;

  std::shared_ptr<Image> getNextImage(VkSemaphore swapchainSemaphore);
  void present(VkSemaphore semaphore) const;
};
