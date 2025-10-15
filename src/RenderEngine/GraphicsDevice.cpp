#include "GraphicsDevice.hpp"

#include "src/RenderEngine/MeshGroup/Mesh.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/Shader.hpp"
#include "src/RenderEngine/MeshGroup/Texture.hpp"
#include "src/Tools/Hashing.hpp"

#include <volk/volk.h>

GraphicsDevice::GraphicsDevice(std::filesystem::path path) {
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
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
    VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .pNext = nullptr,
      .objectType = VK_OBJECT_TYPE_PHYSICAL_DEVICE,
      .objectHandle = reinterpret_cast<uint64_t>(device.physical_device.physical_device),
      .pObjectName = device.physical_device.name.c_str()
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(device.device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
    std::string name = device.physical_device.name + " | Logical Device";
    nameInfo.objectType   = VK_OBJECT_TYPE_DEVICE;
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(device.device);
    nameInfo.pObjectName  = name.c_str();
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(device.device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif
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
  if (const VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &allocator); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create VMA allocator");

  const VkCommandPoolCreateInfo commandPoolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,
    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    .queueFamilyIndex = globalQueueFamilyIndex,
  };
  if (const VkResult result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create command pool");

  yyjson_read_err error;
  graphicsJSON = yyjson_read_file(path.string().c_str(), YYJSON_READ_ALLOW_INF_AND_NAN, nullptr, &error);
  if (graphicsJSON == nullptr) GraphicsInstance::showError("failed to read graphics data JSON: " + std::string(error.msg));
  resourcesDirectory = path.parent_path();
  yyjson_val* root = yyjson_doc_get_root(graphicsJSON);
  JSONTextureArray = yyjson_obj_get(root, "textures");
  JSONTextureArrayCount = yyjson_arr_size(JSONTextureArray);
  JSONOverrideShaders = yyjson_obj_get(root, "overrideShaders");
  JSONShaderArray = yyjson_obj_get(root, "shaders");
  JSONShaderArrayCount = yyjson_arr_size(JSONShaderArray);
  JSONMaterialArray = yyjson_obj_get(root, "materials");
  JSONMaterialArrayCount = yyjson_arr_size(JSONMaterialArray);
  JSONMeshArray = yyjson_obj_get(root, "meshes");
  JSONMeshArrayCount = yyjson_arr_size(JSONMeshArray);
}

GraphicsDevice::~GraphicsDevice() {
  vkDeviceWaitIdle(device);
  vkDestroyCommandPool(device, commandPool, nullptr);
  descriptorSetAllocator.destroy();
  commandPool = VK_NULL_HANDLE;
  meshes.clear();
  if (allocator != VK_NULL_HANDLE) vmaDestroyAllocator(allocator);
  allocator = VK_NULL_HANDLE;
  globalQueue = VK_NULL_HANDLE;
  destroy_device(device);
  yyjson_doc_free(graphicsJSON);
}

VkSampler* GraphicsDevice::getSampler(const VkFilter magnificationFilter, const VkFilter minificationFilter, const VkSamplerMipmapMode mipmapMode, const VkSamplerAddressMode addressMode, const float lodBias, const VkBorderColor borderColor) {
  std::uint64_t id = Tools::hash(magnificationFilter, minificationFilter, mipmapMode, addressMode, lodBias, borderColor);
  if (const auto it = samplers.find(id); it != samplers.end())
    return &it->second;
  const VkSamplerCreateInfo createInfo{
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
  VkSampler* sampler = &samplers.emplace(id, VkSampler()).first->second;
  if (const VkResult result = vkCreateSampler(device, &createInfo, nullptr, sampler); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create sampler");
  return sampler;
}

Shader* GraphicsDevice::getJSONShader(const std::string& name) {
  if (const auto it = overrideShaders.find(name); it != overrideShaders.end()) return it->second;
  return overrideShaders.emplace(name, new Shader(this, resourcesDirectory / "shaders" / yyjson_get_str(yyjson_obj_get(yyjson_obj_get(JSONOverrideShaders, name.c_str()), "path")))).first->second;
}

Shader* GraphicsDevice::getJSONShader(const std::uint64_t id) {
  if (id >= JSONShaderArrayCount) return nullptr;
  if (const auto it = shaders.find(id); it != shaders.end()) return it->second;
  return shaders.emplace(id, new Shader(this, resourcesDirectory / "shaders" / yyjson_get_str(yyjson_obj_get(yyjson_arr_get(JSONShaderArray, id), "path")))).first->second;
}

Texture* GraphicsDevice::getJSONTexture(const std::uint64_t id) {
  if (id >= JSONTextureArrayCount) return nullptr;
  if (const auto it = textures.find(id); it != textures.end()) return it->second;
  CommandBuffer commandBuffer;
  Texture* texture = textures.emplace(id, Texture::jsonGet(this, yyjson_arr_get(JSONTextureArray, id), commandBuffer)).first->second;
  commandBuffer.preprocess();
  executeCommandBufferImmediate(commandBuffer);
  return texture;
}

Pipeline* GraphicsDevice::getPipeline(Material* material, std::uint64_t renderPassCompatibility) {
  const std::uint64_t key = Tools::hash(material, renderPassCompatibility);
  if (const auto it = pipelines.find(key); it != pipelines.end()) return &it->second;
  return &pipelines.emplace(key, Pipeline(this, material)).first->second;
}

Material* GraphicsDevice::getMaterial(const std::uint64_t id, const Material* material) {
  if (const auto it = overrideMaterials.find(id); it != overrideMaterials.end()) return &it->second;
  return &overrideMaterials.emplace(id, Material(*material)).first->second;
}

Material* GraphicsDevice::getMaterial(const std::uint64_t id) {
  if (id >= JSONMaterialArrayCount) return nullptr;
  if (const auto it = materials.find(id); it != materials.end()) return &it->second;
  return &materials.emplace(id, Material(this, yyjson_arr_get(JSONMaterialArray, id))).first->second;
}

Mesh* GraphicsDevice::getJSONMesh(const std::uint64_t id) {
  if (id >= JSONMeshArrayCount) return nullptr;
  if (const auto it = meshes.find(id); it != meshes.end()) return &it->second;
  return &meshes.emplace(id, Mesh(this, yyjson_arr_get(JSONMeshArray, id))).first->second;
}

void GraphicsDevice::update() {
  CommandBuffer commandBuffer;
  for (Mesh& mesh: meshes | std::ranges::views::values) mesh.update(commandBuffer);
  if (!commandBuffer.empty()) executeCommandBufferImmediate(commandBuffer);
}

GraphicsDevice::ImmediateExecutionContext GraphicsDevice::executeCommandBufferAsync(const CommandBuffer& commandBuffer) const {
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
  waitForAsyncCommandBuffer(executeCommandBufferAsync(commandBuffer));
}