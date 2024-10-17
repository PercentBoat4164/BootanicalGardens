#include "Window.hpp"

#include "GraphicsDevice.hpp"

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_vulkan.h>
#include <magic_enum/magic_enum.hpp>

#include <chrono>

uint32_t Window::getImage(VkSemaphore swapchainSemaphore) {
  vkAcquireNextImageKHR(device.device, swapchain.swapchain, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1U)).count(), swapchainSemaphore, VK_NULL_HANDLE, &swapchainIndex);
  return swapchainIndex;
}

Window::Window(const GraphicsDevice& device) : device{device}, renderer{device} {
  window = SDL_CreateWindow("Example Engine", SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0), 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_MOUSE_CAPTURE);
  GraphicsInstance::showError(window, "Failed to create the SDL window.");
  GraphicsInstance::showError(SDL_Vulkan_CreateSurface(window, GraphicsInstance::instance, &surface), "SDL failed to create the Vulkan surface.");
  vkb::SwapchainBuilder builder{device.device, surface};
  builder.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  const vkb::Result<vkb::Swapchain> swapchainResult = builder.build();
  GraphicsInstance::showError(swapchainResult, "Failed to create the Vulkan swapchain.");
  swapchain = swapchainResult.value();
  swapchainImages.reserve(swapchain.image_count);
  std::vector<VkImage> images = swapchain.get_images().value();
  std::vector<VkImageView> views = swapchain.get_image_views().value();
  for (uint32_t i{}; i < swapchain.image_count; ++i) swapchainImages.emplace_back(device, "swapchainImage" + std::to_string(i), images[i], swapchain.image_format, VkExtent3D{swapchain.extent.width, swapchain.extent.height, 1}, swapchain.image_usage_flags, views[i]);
  renderer.setup(swapchainImages);
}

Window::~Window() {
  destroy_swapchain(swapchain);
  swapchainImages.clear();
  swapchain = {};
  vkDestroySurfaceKHR(GraphicsInstance::instance, surface, nullptr);
  surface = VK_NULL_HANDLE;
  SDL_DestroyWindow(window);
  window = nullptr;
}

void Window::draw() {
  if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) return;
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
  vkQueuePresentKHR(device.globalQueue, &presentInfo);
}

SDL_Window* Window::initialize() {
  SDL_InitSubSystem(SDL_INIT_VIDEO);
  return SDL_CreateWindow("", 0, 0, 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);  // SDL automatically loads Vulkan from the system when a Vulkan window is created.
}

void Window::cleanupInitialization(SDL_Window* window) {
  SDL_DestroyWindow(window);
}