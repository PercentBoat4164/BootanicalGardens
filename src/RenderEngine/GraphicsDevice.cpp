#include "GraphicsDevice.hpp"

#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Shader.hpp"
#include "src/Tools/Hashing.hpp"

#include <volk/volk.h>

void getQueues(VkPhysicalDevice physicalDevice, const std::vector<VkQueueFlags>& required, const std::vector<VkQueueFlags>& requested) {
  uint32_t count{};
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  std::vector<VkQueueFamilyProperties> properties(count);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, properties.data());
  std::vector<unsigned> unallocatedQueues(count);
  for (VkQueueFamilyProperties property : properties) {
    unallocatedQueues.push_back(property.queueCount);
  }
}

GraphicsDevice::GraphicsDevice() :
    perFrameDescriptorAllocator(*this),
    perPassDescriptorAllocator (*this),
    perMaterialDescriptorAllocator(*this),
    perMeshDescriptorAllocator(*this) {
  vkb::PhysicalDeviceSelector deviceSelector{GraphicsInstance::instance};
  deviceSelector.defer_surface_initialization();
  deviceSelector.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);
  vkb::DeviceBuilder deviceBuilder{deviceSelector.select().value()};
  /**@todo: Do not hardcode the queues. Build an actually good algorithm to find the most suitable queues.
   *    Assign badness to each queue family choice based on how many other capabilities that queue family has and how rare those capabilities are on the device.*/
  deviceBuilder.custom_queue_setup({vkb::CustomQueueDescription(0, {1}), vkb::CustomQueueDescription(1, {0})});
  const auto builderResult = deviceBuilder.build();
  if (!builderResult.has_value()) GraphicsInstance::showError(builderResult.vk_result(), "Failed to create the Vulkan device");
  device = builderResult.value();
  volkLoadDevice(device.device);
  const vkb::Result<VkQueue> queueResult = device.get_queue(vkb::QueueType::graphics);
  if (!queueResult.has_value()) GraphicsInstance::showError(queueResult.vk_result(), "Failed to get Vulkan device queue");
  globalQueue = queueResult.value();
  const vkb::Result<uint32_t> queueIndexResult = device.get_queue_index(vkb::QueueType::graphics);
  if (!queueIndexResult.has_value()) GraphicsInstance::showError(queueIndexResult.vk_result(), "Failed to get Vulkan device queue index");
  globalQueueFamilyIndex = queueIndexResult.value();

  VmaVulkanFunctions vulkanFunctions {
    .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
    .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
    .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
    .vkAllocateMemory = vkAllocateMemory,
    .vkFreeMemory = vkFreeMemory,
    .vkMapMemory = vkMapMemory,
    .vkUnmapMemory = vkUnmapMemory,
    .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
    .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
    .vkBindBufferMemory = vkBindBufferMemory,
    .vkBindImageMemory = vkBindImageMemory,
    .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
    .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
    .vkCreateBuffer = vkCreateBuffer,
    .vkDestroyBuffer = vkDestroyBuffer,
    .vkCreateImage = vkCreateImage,
    .vkDestroyImage = vkDestroyImage,
    .vkCmdCopyBuffer = vkCmdCopyBuffer,
    .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
    .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
    .vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
    .vkBindImageMemory2KHR = vkBindImageMemory2KHR,
    .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
    .vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
    .vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements
  };
  VmaAllocatorCreateInfo allocatorCreateInfo {
    .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
    .physicalDevice = device.physical_device,
    .device = device,
    .pVulkanFunctions = &vulkanFunctions,
    .instance = GraphicsInstance::instance,
    .vulkanApiVersion = std::min(device.instance_version, volkGetInstanceVersion()),
  };
  if (const VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &allocator); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create VMA allocator.");

  const VkCommandPoolCreateInfo commandPoolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    .queueFamilyIndex = globalQueueFamilyIndex,
  };
  vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

  perFrameDescriptorAllocator.prepareAllocation({
      /// Frame number, Game time
      VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
  });
  perPassDescriptorAllocator.prepareAllocation({
      /// view * projection matrix
      VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
  });
  perMaterialDescriptorAllocator.prepareAllocation({
      /// Color Texture
      VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
  }),
  perMeshDescriptorAllocator.prepareAllocation({
      /// model matrix
      VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
  });
}

GraphicsDevice::~GraphicsDevice() {
  destroy();
}

GraphicsDevice::ImmediateExecutionContext GraphicsDevice::executeCommandBufferImmediateAsync(const CommandBuffer& commandBuffer) const {
  const VkCommandBufferAllocateInfo allocateInfo{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext              = nullptr,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  VkCommandBuffer vkCmdBuf;
  if (const VkResult result = vkAllocateCommandBuffers(device, &allocateInfo, &vkCmdBuf); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to allocate VkCommandBuffer");
  constexpr VkCommandBufferBeginInfo beginInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext            = nullptr,
      .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
  };
  if (const VkResult result = vkBeginCommandBuffer(vkCmdBuf, &beginInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to begin VkCommandBuffer");
  commandBuffer.bake(vkCmdBuf);
  if (const VkResult result = vkEndCommandBuffer(vkCmdBuf); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to end VkCommandBuffer");
  std::array<VkPipelineStageFlags, 1> arr{VK_PIPELINE_STAGE_NONE};
  const VkSubmitInfo submitInfo{
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext                = nullptr,
      .waitSemaphoreCount   = 0,
      .pWaitSemaphores      = nullptr,
      .pWaitDstStageMask    = arr.data(),
      .commandBufferCount   = 1,
      .pCommandBuffers      = &vkCmdBuf,
      .signalSemaphoreCount = 0,
      .pSignalSemaphores    = nullptr
  };
  constexpr VkFenceCreateInfo fenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
  };
  VkFence fence;
  if (const VkResult result = vkCreateFence(device, &fenceCreateInfo, nullptr, &fence); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create VkFence");
  if (const VkResult result = vkQueueSubmit(globalQueue, 1, &submitInfo, fence); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed VkQueue submission");
  return {vkCmdBuf, fence};
}

void GraphicsDevice::waitForAsyncCommandBuffer(const ImmediateExecutionContext context) const {
  if (const VkResult result = vkWaitForFences(device, 1, &context.fence, VK_TRUE, -1ULL); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to wait for VkFence");
  vkFreeCommandBuffers(device, commandPool, 1, &context.commandBuffer);
  vkDestroyFence(device, context.fence, nullptr);
}

void GraphicsDevice::executeCommandBufferImmediate(const CommandBuffer& commandBuffer) const {
  waitForAsyncCommandBuffer(executeCommandBufferImmediateAsync(commandBuffer));
}

std::shared_ptr<VkSampler> GraphicsDevice::getSampler(const VkFilter magnificationFilter, const VkFilter minificationFilter, const VkSamplerMipmapMode mipmapMode, const VkSamplerAddressMode addressMode, const float lodBias, const VkBorderColor borderColor) {
  std::size_t samplerID = hash(magnificationFilter, minificationFilter, mipmapMode, addressMode, lodBias, borderColor);
  if (const auto it = samplers.find(samplerID); it != samplers.end())
    if (!it->second.expired()) return it->second.lock();
    else samplers.erase(it);
  const VkSamplerCreateInfo createInfo {
      .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext                   = nullptr,
      .flags                   = 0,
      .magFilter               = magnificationFilter,
      .minFilter               = minificationFilter,
      .mipmapMode              = mipmapMode,
      .addressModeU            = addressMode,
      .addressModeV            = addressMode,
      .addressModeW            = addressMode,
      .mipLodBias              = lodBias,
      .anisotropyEnable        = VK_FALSE,
      .maxAnisotropy           = 0,
      .compareEnable           = VK_FALSE,
      .compareOp               = VK_COMPARE_OP_NEVER,
      .minLod                  = std::numeric_limits<float>::min(),
      .maxLod                  = std::numeric_limits<float>::max(),
      .borderColor             = borderColor,
      .unnormalizedCoordinates = VK_FALSE
  };
  auto sampler = std::shared_ptr<VkSampler>(new VkSampler, [this, samplerID](VkSampler* sampler) {
    vkDestroySampler(device, *sampler, nullptr);
    samplers.erase(samplerID);
  });
  samplers.emplace(samplerID, sampler);
  if (const VkResult result = vkCreateSampler(device, &createInfo, nullptr, sampler.get()); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create sampler.");
  return sampler;
}

void GraphicsDevice::destroy() {
  samplers.clear();
  vkDestroyCommandPool(device, commandPool, nullptr);
  commandPool = VK_NULL_HANDLE;
  perFrameDescriptorAllocator.destroy();
  perPassDescriptorAllocator.destroy();
  perMaterialDescriptorAllocator.destroy();
  perMeshDescriptorAllocator.destroy();
  if (allocator != VK_NULL_HANDLE) vmaDestroyAllocator(allocator);
  allocator = VK_NULL_HANDLE;
  if (device) {
    vkDeviceWaitIdle(device);
    destroy_device(device);
  }
  globalQueue = VK_NULL_HANDLE;
}