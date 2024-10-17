#include "GraphicsDevice.hpp"

#include "GraphicsInstance.hpp"

#include <volk.h>

void getQueues(VkPhysicalDevice physicalDevice, std::vector<VkQueueFlags> required, std::vector<VkQueueFlags> requested) {
  uint32_t count{};
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
  std::vector<VkQueueFamilyProperties> properties(count);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, properties.data());
  std::vector<unsigned> unallocatedQueues(count);
  for (VkQueueFamilyProperties property : properties) {
    unallocatedQueues.push_back(property.queueCount);
  }
}

GraphicsDevice::GraphicsDevice() {
  vkb::PhysicalDeviceSelector deviceSelector{GraphicsInstance::instance};
  deviceSelector.defer_surface_initialization();
  deviceSelector.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);
  vkb::DeviceBuilder deviceBuilder{deviceSelector.select().value()};
  /**@todo Do not hardcode the queues. Build an actually good algorithm to find the most suitable queues.
   *    Assign badness to each queue family choice based on how many other capabilities that queue family has and how rare those capabilities are on the device.*/
  deviceBuilder.custom_queue_setup({vkb::CustomQueueDescription(0, {1}), vkb::CustomQueueDescription(1, {0})});
  const auto builderResult = deviceBuilder.build();
  GraphicsInstance::showError(builderResult, "Failed to create the Vulkan device.");
  device = builderResult.value();
  volkLoadDevice(device.device);
  const vkb::Result<VkQueue> queueResult = device.get_queue(vkb::QueueType::graphics);
  GraphicsInstance::showError(queueResult, "Failed to get Vulkan device queue.");
  globalQueue = queueResult.value();
  const vkb::Result<uint32_t> queueIndexResult = device.get_queue_index(vkb::QueueType::graphics);
  GraphicsInstance::showError(queueIndexResult, "Failed to get Vulkan device queue index");
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
  GraphicsInstance::showError(vmaCreateAllocator(&allocatorCreateInfo, &allocator), "Failed to create VMA allocator.");

  const VkCommandPoolCreateInfo commandPoolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    .queueFamilyIndex = globalQueueFamilyIndex,
  };
  vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
}

GraphicsDevice::~GraphicsDevice() {
  destroy();
}

VkCommandBuffer GraphicsDevice::getOneShotCommandBuffer() const {
  const VkCommandBufferAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = nullptr,
    .commandPool = commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);
  constexpr VkCommandBufferBeginInfo beginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo = nullptr,
  };
  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

VkFence GraphicsDevice::submitOneShotCommandBuffer(VkCommandBuffer commandBuffer) const {
  vkEndCommandBuffer(commandBuffer);
  std::array<VkPipelineStageFlags, 1> arr{VK_PIPELINE_STAGE_NONE};
  const VkSubmitInfo submitInfo {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = nullptr,
    .pWaitDstStageMask = arr.data(),
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
    .signalSemaphoreCount = 0,
    .pSignalSemaphores = nullptr
  };
  constexpr VkFenceCreateInfo fenceCreateInfo {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
  };
  VkFence fence;
  vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
  vkQueueSubmit(globalQueue, 1, &submitInfo, fence);
  return fence;
}

void GraphicsDevice::destroy() {
  vkDestroyCommandPool(device, commandPool, nullptr);
  commandPool = VK_NULL_HANDLE;
  if (allocator != VK_NULL_HANDLE) vmaDestroyAllocator(allocator);
  allocator = VK_NULL_HANDLE;
  if (device) {
    vkDeviceWaitIdle(device);
    destroy_device(device);
  }
  std::construct_at(&device);
}