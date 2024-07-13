#include "Window.hpp"

#include "GraphicsDevice.hpp"

#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_init.h>
#include <magic_enum/magic_enum.hpp>

#include <chrono>

uint32_t Window::getImage(VkSemaphore swapchainSemaphore) {
  vkAcquireNextImageKHR(device.device, swapchain.swapchain, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1U)).count(), swapchainSemaphore, VK_NULL_HANDLE, &swapchainIndex);
  return swapchainIndex;
}

Window::Window(const GraphicsDevice& device) : device{device}, renderer{device} {
  window = SDL_CreateWindow("Example Engine", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_MOUSE_CAPTURE);
  GraphicsInstance::showError(window, "Failed to create the SDL window.");
  GraphicsInstance::showError(SDL_Vulkan_CreateSurface(window, GraphicsInstance::instance, nullptr, &surface), "SDL failed to create the Vulkan surface.");
  vkb::SwapchainBuilder builder{device.device, surface};
  builder.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  const vkb::Result<vkb::Swapchain> swapchainResult = builder.build();
  GraphicsInstance::showError(swapchainResult, "Failed to create the Vulkan swapchain.");
  swapchain = swapchainResult.value();
  swapchainImages.reserve(swapchain.image_count);
  std::vector<VkImage> images = swapchain.get_images().value();
  std::vector<VkImageView> views = swapchain.get_image_views().value();
  for (uint32_t i{}; i < swapchain.image_count; ++i) swapchainImages.emplace_back(device, "swapchainImage" + std::to_string(i), images[i], swapchain.image_format, swapchain.extent, swapchain.image_usage_flags, views[i]);
  renderer.setup(swapchainImages);
}

Window::~Window() {
  if (swapchain) {
    destroy_swapchain(swapchain);
    swapchainImages.clear();
  }
  if (surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(GraphicsInstance::instance, surface, nullptr);
    surface = VK_NULL_HANDLE;
  }
  if (window != nullptr) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }
}

void Window::draw() {
  std::vector<VkSemaphore> waitSemaphores = renderer.render(getImage(renderer.waitForFrameData()), swapchainImages[swapchainIndex]);
  const VkPresentInfoKHR presentInfo {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = nullptr,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &swapchain.swapchain,
      .pImageIndices = &swapchainIndex,
      .pResults = nullptr
  };
  vkQueuePresentKHR(device.queue, &presentInfo);
}

void Window::initialize() {
  SDL_InitSubSystem(SDL_INIT_VIDEO);
  SDL_DestroyWindow(SDL_CreateWindow("", 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN));
}