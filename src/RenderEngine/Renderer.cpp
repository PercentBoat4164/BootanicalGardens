#include "Renderer.hpp"

#include "GraphicsDevice.hpp"
#include "Image.hpp"
#include "Shader.hpp"
#include "Window.hpp"

#include <chrono>
#include <cmath>
#include <vector>

Renderer::PerFrameData::PerFrameData(const GraphicsDevice& device) : device(device) {
  /****************************************
   * Initialize Command Recording Objects *
   ****************************************/
  const VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = device.globalQueueFamilyIndex
  };
  GraphicsInstance::showError(vkCreateCommandPool(device.device, &commandPoolCreateInfo, nullptr, &commandPool), "Failed to create command pool.");
  const VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext              = nullptr,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
  };
  GraphicsInstance::showError(vkAllocateCommandBuffers(device.device, &commandBufferAllocateInfo, &commandBuffer), "Failed to allocate command buffers.");

  /*****************************************
   * Initialize Synchronization Objections *
   *****************************************/
  constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
  };
  GraphicsInstance::showError(vkCreateSemaphore(device.device, &semaphoreCreateInfo, nullptr, &swapchainSemaphore), "Failed to create semaphore.");
  GraphicsInstance::showError(vkCreateSemaphore(device.device, &semaphoreCreateInfo, nullptr, &renderSemaphore), "Failed to create semaphore.");
  constexpr VkFenceCreateInfo fenceCreateInfo {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };
  GraphicsInstance::showError(vkCreateFence(device.device, &fenceCreateInfo, nullptr, &renderFence), "Failed to create fence");
}

Renderer::PerFrameData::~PerFrameData() {
  if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device.device, commandPool, nullptr);
  if (renderSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device.device, renderSemaphore, nullptr);
  if (swapchainSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device.device, swapchainSemaphore, nullptr);
  if (renderFence != VK_NULL_HANDLE) vkDestroyFence(device.device, renderFence, nullptr);
}

const Renderer::PerFrameData& Renderer::getPerFrameData() const {
  return frames[frameNumber % FRAME_OVERLAP];
}

Renderer::OneTimeSubmit::CommandBuffer::CommandBuffer(const GraphicsDevice& device, VkCommandPool commandPool) : device(device) {
  const VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext              = nullptr,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
  };
  GraphicsInstance::showError(vkAllocateCommandBuffers(device.device, &commandBufferAllocateInfo, &commandBuffer), "Failed to allocate one-time-submit commandbuffer.");
}

Renderer::OneTimeSubmit::CommandBuffer Renderer::OneTimeSubmit::CommandBuffer::begin() {
  constexpr VkCommandBufferBeginInfo commandBufferBeginInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext            = nullptr,
      .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr
  };
  GraphicsInstance::showError(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo), "Failed to begin one-time-submit commandbuffer.");
  inUse = true;
  return *this;
}

void Renderer::OneTimeSubmit::CommandBuffer::finish() {
  vkEndCommandBuffer(commandBuffer);
  VkFence fence;
  constexpr VkFenceCreateInfo fenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
  };
  vkCreateFence(device.device, &fenceCreateInfo, nullptr, &fence);
  const VkSubmitInfo submitInfo{
      .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext              = nullptr,
      .commandBufferCount = 1,
      .pCommandBuffers    = &commandBuffer,
  };
  vkQueueSubmit(device.globalQueue, 1, &submitInfo, fence);
  vkWaitForFences(device.device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkDestroyFence(device.device, fence, nullptr);
  vkResetCommandBuffer(commandBuffer, 0);
  inUse = false;
}

Renderer::OneTimeSubmit::CommandBuffer::operator VkCommandBuffer() const {
  return commandBuffer;
}

Renderer::OneTimeSubmit::CommandBuffer::operator bool() const {
  return inUse;
}

Renderer::OneTimeSubmit::CommandBuffer Renderer::OneTimeSubmit::get() {
  const VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = device.globalQueueFamilyIndex
  };
  GraphicsInstance::showError(vkCreateCommandPool(device.device, &commandPoolCreateInfo, nullptr, &commandPool), "Failed to create one-time-submit command pool.");
  for (CommandBuffer& commandBuffer: commandBuffers)
    if (!commandBuffer) return commandBuffer.begin();
  return commandBuffers.emplace_back(device, commandPool).begin();
}

Renderer::OneTimeSubmit::OneTimeSubmit(const GraphicsDevice& device) : device(device) {}

Renderer::OneTimeSubmit::~OneTimeSubmit() {
  vkDestroyCommandPool(device.device, commandPool, nullptr);
  commandPool = VK_NULL_HANDLE;
}

Renderer::Renderer(const GraphicsDevice& device) : device(device), oneTimeSubmit(device), drawImage(device, "drawImage"), descriptorAllocator(device) {
  frames.reserve(FRAME_OVERLAP);
  for (uint32_t i{}; i < FRAME_OVERLAP; ++i)
    frames.emplace_back(device);
}

Renderer::~Renderer() {
  const PerFrameData& frameData = getPerFrameData();
  vkWaitForFences(device.device, 1, &frameData.renderFence, VK_TRUE, UINT64_MAX);
}

void Renderer::setup(const std::vector<Image>& swapchainImages) {
  drawImage.buildInPlace(VK_FORMAT_R16G16B16A16_SFLOAT, {swapchainImages.back().extent()}, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  // shaders.emplace_back(device, "../res/gradient.comp");
  // shaderUsages.emplace_back(device, shaders.back(), std::unordered_map<std::string, Resource&>{{"image", drawImage}}, *this);
  // pipelines.emplace_back(device, std::vector{&shaderUsages.back()});

//  shaders.emplace_back(device);
//  shaders.emplace_back(device);
  // shaderUsages.emplace_back(device, shaders[0], std::unordered_map<std::string, Resource&>{}, *this);
  // shaderUsages.emplace_back(device, shaders[1], std::unordered_map<std::string, Resource&>{}, *this);
  // pipelines.emplace_back(device, std::vector{&shaderUsages[0], &shaderUsages[1]});
  renderPasses.emplace_back(device, std::vector{&pipelines.back()});

  OneTimeSubmit::CommandBuffer commandBuffer = oneTimeSubmit.get();
  std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
  imageMemoryBarriers.reserve(swapchainImages.size());
  for (const Image& image : swapchainImages)
    imageMemoryBarriers.emplace_back(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, device.globalQueueFamilyIndex, device.globalQueueFamilyIndex, image.image(), VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
  vkCmdPipelineBarrier(static_cast<VkCommandBuffer>(commandBuffer), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, imageMemoryBarriers.size(), imageMemoryBarriers.data());
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, device.globalQueueFamilyIndex, device.globalQueueFamilyIndex, drawImage.image(), VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
  vkCmdPipelineBarrier(static_cast<VkCommandBuffer>(commandBuffer), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  commandBuffer.finish();
}

VkSemaphore Renderer::waitForFrameData() const {
  const PerFrameData& frameData = getPerFrameData();
  GraphicsInstance::showError(vkWaitForFences(device.device, 1, &frameData.renderFence, true, UINT64_MAX), "Failed to wait for fence.");
  GraphicsInstance::showError(vkResetFences(device.device, 1, &frameData.renderFence), "Failed to reset fence.");
  return frameData.swapchainSemaphore;
}

std::vector<VkSemaphore> Renderer::render(const uint32_t swapchainIndex, Image& swapchainImage) const {
  const PerFrameData& frameData = getPerFrameData();
  GraphicsInstance::showError(vkResetCommandBuffer(frameData.commandBuffer, 0), "Failed to reset commandbuffer.");
  constexpr VkCommandBufferBeginInfo commandBufferBeginInfo {
    .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext            = nullptr,
    .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = nullptr
  };
  GraphicsInstance::showError(vkBeginCommandBuffer(frameData.commandBuffer, &commandBufferBeginInfo), "Failed to begin commandbuffer recording.");
  std::vector<VkImageMemoryBarrier> imageMemoryBarriers {
      {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, device.globalQueueFamilyIndex, device.globalQueueFamilyIndex, drawImage.image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
      // Render into drawImage
      {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, device.globalQueueFamilyIndex, device.globalQueueFamilyIndex, drawImage.image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
      {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device.globalQueueFamilyIndex, device.globalQueueFamilyIndex, swapchainImage.image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
      // Blit drawImage to swapchainImage
      {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, device.globalQueueFamilyIndex, device.globalQueueFamilyIndex, swapchainImage.image(), {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
  };
  vkCmdPipelineBarrier(frameData.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarriers[0]);
  for (const auto& renderPass : renderPasses)
    renderPass.render(frameData.commandBuffer, {{0, 0}, drawImage.extent().width, drawImage.extent().height}, {{0.0F, static_cast<float>(std::sin(static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) / 1000000.0)) / 2.0F + 0.5F, 0.0F, 0.0F}}, swapchainIndex);
  vkCmdPipelineBarrier(frameData.commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarriers[1]);
  vkCmdPipelineBarrier(frameData.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarriers[2]);
  const VkImageBlit imageBlit {
      .srcSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .srcOffsets = {{0, 0, 0}, {static_cast<int32_t>(drawImage.extent().width), static_cast<int32_t>(drawImage.extent().height), 1}},
      .dstSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .dstOffsets = {{0, 0, 0}, {static_cast<int32_t>(swapchainImage.extent().width), static_cast<int32_t>(swapchainImage.extent().height), 1}},
  };
  vkCmdBlitImage(frameData.commandBuffer, drawImage.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchainImage.image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_NEAREST);
  vkCmdPipelineBarrier(frameData.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarriers[3]);
  GraphicsInstance::showError(vkEndCommandBuffer(frameData.commandBuffer), "Failed to end commandbuffer recording.");
  std::array<VkPipelineStageFlags, 1> stageMasks{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
  const VkSubmitInfo submitInfo {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &frameData.swapchainSemaphore,
    .pWaitDstStageMask = stageMasks.data(),
    .commandBufferCount = 1,
    .pCommandBuffers = &frameData.commandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &frameData.renderSemaphore
  };
  GraphicsInstance::showError(vkQueueSubmit(device.globalQueue, 1, &submitInfo, frameData.renderFence), "Failed to submit recorded commandbuffer to queue.");
  return {frameData.renderSemaphore};
}