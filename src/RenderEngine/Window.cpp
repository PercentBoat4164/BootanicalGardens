#include "Window.hpp"

#include "GraphicsDevice.hpp"

#include "Resources/Image.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <volk/volk.h>

#include <chrono>

Window::Window(GraphicsDevice* const device) : device{device} {
  window = SDL_CreateWindow("Bootanical Gardens", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_MOUSE_CAPTURE);
  if (window == nullptr) GraphicsInstance::showSDLError();
  if (!SDL_Vulkan_CreateSurface(window, GraphicsInstance::instance, nullptr, &surface)) GraphicsInstance::showSDLError();
  vkb::SwapchainBuilder builder{device->device, surface};
  builder.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  int width, height;
  SDL_GetWindowSizeInPixels(window, &width, &height);
  builder.set_desired_extent(width, height);
  builder.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR);
  const vkb::Result<vkb::Swapchain> swapchainResult = builder.build();
  if (!swapchainResult.has_value()) GraphicsInstance::showError(swapchainResult.vk_result(), "failed to create the swapchain");
  swapchain = swapchainResult.value();
  swapchainImages.reserve(swapchain.image_count);
  std::vector<VkImage> images = swapchain.get_images().value();
  std::vector<VkImageView> views = swapchain.get_image_views().value();
  for (uint32_t i{}; i < swapchain.image_count; ++i)
    swapchainImages.emplace_back(std::make_shared<Image>(device, "swapchainImage" + std::to_string(i), images[i], swapchain.image_format, VkExtent3D{swapchain.extent.width, swapchain.extent.height, 1}, swapchain.image_usage_flags, 1, VK_SAMPLE_COUNT_1_BIT, views[i]));
  VkSemaphoreCreateInfo semaphoreCreateInfo{
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0
  };
  for (uint32_t i{}; i < swapchain.image_count; ++i) {
    semaphores.emplace_back(VK_NULL_HANDLE);
    if (const VkResult result = vkCreateSemaphore(device->device, &semaphoreCreateInfo, nullptr, &semaphores.back()); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create the semaphore");
  }
}

Window::~Window() {
  for (VkSemaphore semaphore: semaphores) vkDestroySemaphore(device->device, semaphore, nullptr);
  semaphores.clear();
  destroy_swapchain(swapchain);
  swapchainImages.clear();
  swapchain = {};
  vkDestroySurfaceKHR(GraphicsInstance::instance, surface, nullptr);
  surface = VK_NULL_HANDLE;
  SDL_DestroyWindow(window);
  window = nullptr;
}

VkExtent3D Window::getResolution() const {
  return VkExtent3D{swapchain.extent.width, swapchain.extent.height, 1};
}

std::shared_ptr<Image> Window::getNextImage(VkSemaphore swapchainSemaphore) {
  if (const VkResult result = vkAcquireNextImageKHR(device->device, swapchain.swapchain, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1U)).count(), swapchainSemaphore, VK_NULL_HANDLE, &swapchainIndex); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to acquire swapchain image");
  return swapchainImages[swapchainIndex];
}

VkSemaphore Window::getSemaphore() const {
  return semaphores[swapchainIndex];
}

void Window::present() const {
  const VkPresentInfoKHR presentInfo{
      .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext              = nullptr,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores    = &semaphores[swapchainIndex],
      .swapchainCount     = 1,
      .pSwapchains        = &swapchain.swapchain,
      .pImageIndices      = &swapchainIndex,
      .pResults           = nullptr
  };
  if (const VkResult result = vkQueuePresentKHR(device->globalQueue, &presentInfo); result != VK_SUCCESS) return GraphicsInstance::showError(result, "failed to present");
}

/**
 * This <em>must</em> be called from the main thread.
 * @return The new SDL_Window handle
 */
SDL_Window* Window::initialize() {
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) GraphicsInstance::showSDLError();
  if (!SDL_Vulkan_LoadLibrary(nullptr)) GraphicsInstance::showSDLError();
  SDL_Window* window = SDL_CreateWindow("", 1, 1, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
  if (window == nullptr) GraphicsInstance::showSDLError();
  return window;
}

void Window::cleanupInitialization(SDL_Window* window) {
  SDL_DestroyWindow(window);
}