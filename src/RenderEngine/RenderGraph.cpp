#include "RenderGraph.hpp"

#include "Renderable/Texture.hpp"
#include "src/Game/Game.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Mesh.hpp"
#include "src/RenderEngine/Renderable/MeshGroup.hpp"
#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"
#include "src/RenderEngine/Window.hpp"
#include "src/Tools/Hashing.hpp"

#include <volk/volk.h>

#include <ranges>
#include <vector>

RenderGraph::PerFrameData::PerFrameData(GraphicsDevice* const device, const RenderGraph& graph) : device(device), graph(graph) {
  /****************************************
   * Initialize Command Recording Objects *
   ****************************************/
  const VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = device->globalQueueFamilyIndex
  };
  if (const VkResult result = vkCreateCommandPool(device->device, &commandPoolCreateInfo, nullptr, &commandPool); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create command pool");
  const VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext              = nullptr,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
  };
  if (const VkResult result = vkAllocateCommandBuffers(device->device, &commandBufferAllocateInfo, &commandBuffer); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to allocate command buffers");

  /**************************************
   * Initialize Synchronization Objects *
   **************************************/
  constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
  };
  if (const VkResult result = vkCreateSemaphore(device->device, &semaphoreCreateInfo, nullptr, &frameDataSemaphore); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create semaphore");
  if (const VkResult result = vkCreateSemaphore(device->device, &semaphoreCreateInfo, nullptr, &frameFinishedSemaphore); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create semaphore");
  constexpr VkFenceCreateInfo fenceCreateInfo {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };
  if (const VkResult result = vkCreateFence(device->device, &fenceCreateInfo, nullptr, &renderFence); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create fence");
}

RenderGraph::PerFrameData::~PerFrameData() {
  vkDestroyCommandPool(device->device, commandPool, nullptr);
  vkQueueWaitIdle(device->globalQueue);
  vkDestroySemaphore(device->device, frameFinishedSemaphore, nullptr);
  vkDestroySemaphore(device->device, frameDataSemaphore, nullptr);
  vkDestroyFence(device->device, renderFence, nullptr);
}

uint8_t RenderGraph::FRAMES_IN_FLIGHT = 2;

RenderGraph::RenderGraph(GraphicsDevice* const device) : device(device) {
  // Build frame data
  frames.reserve(FRAMES_IN_FLIGHT);
  for (uint32_t i{}; i < FRAMES_IN_FLIGHT; ++i) frames.emplace_back(device, *this);

  uniformBuffer = std::make_shared<UniformBuffer<GraphData>>(device, "RenderGraph UniformBuffer");
}

RenderGraph::~RenderGraph() {
  // Wait for all submitted frames in this graph to finish rendering so that the resources can be freed.
  std::vector<VkFence> fences(frames.size());
  for (uint32_t i{}; i < frames.size(); ++i) fences[i] = frames[i].renderFence;
  if (const VkResult result = vkWaitForFences(device->device, fences.size(), fences.data(), VK_TRUE, std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1U)).count()); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to wait for fences");
}

void RenderGraph::setResolutionGroup(const std::string_view& name, const VkExtent3D resolution, const VkSampleCountFlags sampleCount) {
  ResolutionGroupProperties& group = resolutionGroups[getResolutionGroupId(name)];
  group.resolution = resolution;
  group.sampleCount = sampleCount;
  for (ImageID attachmentId : group.attachments) if (std::shared_ptr<Image> image = images[attachmentId].image; image != nullptr) image->rebuild(resolution, sampleCount);
  outOfDate = true;
}

void RenderGraph::setImage(const std::string_view& name, const std::string_view& groupName, const VkFormat format, const bool inheritSampleCount) {
  const ImageID id = getImageId(name);
  const ResolutionGroupID groupId = getResolutionGroupId(groupName);
  ImageProperties& image = images[id];
  image = {
    .resolutionGroup = groupId,
    .format = format,
    .inheritSampleCount = inheritSampleCount,
    .image = image.image,
    .name = std::string(name.empty() ? image.name : name)
  };
  if (plf::colony<ImageID>& resolutionGroupAttachments = resolutionGroups.at(groupId).attachments; std::ranges::find(resolutionGroupAttachments, id) == resolutionGroupAttachments.end()) resolutionGroupAttachments.insert(id);
  outOfDate = true;
}

RenderGraph::ImageProperties RenderGraph::getImage(const ImageID id) const {
  return images.at(id);
}

bool RenderGraph::hasImage(const ImageID id) const {
  return images.contains(id);
}

bool RenderGraph::combineImageAccesses(ImageAccess& dst, const ImageAccess& src) {
  if (dst.layout != src.layout) return false;  // Layouts must match
  dst.usage |= src.usage;
  dst.access |= src.access;
  dst.stage |= src.stage;
  return true;
}

/**
 * Guarantees that the RenderGraph is baked. If the RenderGraph is not out-of-date, then no baking will actually occur.
 *
 * @todo: Needs heavy optimization. This function's high speed will be crucial for avoiding hitches or objects popping in. This function should be heavily threaded.
 *
 * @return <c>true</c> if baking actually happened, <c>false</c> otherwise.
 */
bool RenderGraph::bake() {
  if (!outOfDate) return false;
  /*************************
   * Process Render Passes *
   *************************/
  {
    // Understand how attachments are used across RenderPasses
    std::unordered_map<RenderPass*, std::vector<ImageID>> pass2id;
    std::unordered_map<ImageID, std::vector<std::pair<RenderPass*, ImageAccess>>> id2decl;
    std::unordered_map<ImageID, VkImageUsageFlags> usages {
      {getImageId(RenderColor), VK_IMAGE_USAGE_TRANSFER_SRC_BIT}
    };
    for (const std::shared_ptr<RenderPass>& renderPass : renderPasses) {
      std::vector<std::pair<ImageID, ImageAccess>> accesses = renderPass->declareAccesses();
      std::vector<ImageID> ids;
      for (auto& [id, access] : accesses) {
        usages[id] |= access.usage;
        if (!images.contains(id))
          continue;
        id2decl[id].emplace_back(renderPass.get(), access);
        ids.push_back(id);
      }
      pass2id[renderPass.get()] = ids;
    }
    buildImages(usages);

    // Bake RenderPasses
    for (const std::shared_ptr<RenderPass>& renderPass : renderPasses) {
      const std::vector<ImageID>& renderPassAttachmentIDs = pass2id.at(renderPass.get());
      std::vector<VkAttachmentDescription> descriptions;
      descriptions.reserve(renderPassAttachmentIDs.size());
      std::vector<std::shared_ptr<Image>> attachments;
      attachments.reserve(renderPassAttachmentIDs.size());
      for (const ImageID& id: renderPassAttachmentIDs) {
        /**@todo: Add support for aliasing attachments.*/
        /**@todo: Add support for reordering render passes.*/
        std::shared_ptr<Image> image = getImage(id).image;
        std::vector<std::pair<RenderPass*, ImageAccess>>& attachmentDeclarations = id2decl.at(id);
        // Find this renderpass in the declarations of this attachment.
        const auto thisIt = std::ranges::find(attachmentDeclarations, renderPass.get(), &decltype(id2decl)::mapped_type::value_type::first);
        /**@todo: Optimize load and store ops.*/
        /**@todo: Log an error if the format does not include a stencil buffer, but the stencilLoadOp or stencilStoreOp are not DONT_CARE.*/
        /**@todo: Log an error if this the loadOp is CLEAR and the attachment is an input attachment.*/
        const ImageAccess& thisDeclaration = thisIt->second;
        descriptions.push_back({
            .flags = 0U,
            .format = image->getFormat(),
            .samples = static_cast<VkSampleCountFlagBits>(image->getSampleCount()),
            .loadOp = thisDeclaration.loadOp,
            .storeOp = thisDeclaration.storeOp,
            .stencilLoadOp = thisDeclaration.stencilLoadOp,
            .stencilStoreOp = thisDeclaration.stencilStoreOp,
            .initialLayout = thisDeclaration.layout,
            .finalLayout = thisDeclaration.layout
        });
        attachments.push_back(image);
      }
      renderPass->bake(descriptions, attachments);
      if (renderPass->compatibility == -1U) GraphicsInstance::showError("`RenderPass::bake(...)` failed to update the RenderPass compatibility.");
    }

    for (const std::shared_ptr<RenderPass>& renderPass : renderPasses) {
      for (const std::shared_ptr<Material>& material: renderPass->getPipelines() | std::views::keys) {
        if (std::shared_ptr<Texture> albedo = material->getAlbedoTexture(); albedo != nullptr) {
          std::string name = material->getAlbedoTextureName();
          images[getImageId(name)] = {
            .resolutionGroup = getResolutionGroupId(VoidResolutionGroup),
            .format = albedo->getFormat(),
            .inheritSampleCount = false,
            .image = albedo,
            .name = name
          };
        }
        if (std::shared_ptr<Texture> normal = material->getNormalTexture(); normal != nullptr) {
          std::string name = material->getNormalTextureName();
          images[getImageId(name)] = {
            .resolutionGroup = getResolutionGroupId(VoidResolutionGroup),
            .format = normal->getFormat(),
            .inheritSampleCount = false,
            .image = normal,
            .name = name
          };
        }
      }
    }
  }

  /***************************
   * Process Descriptor Sets *
   ***************************/
  // Compute the descriptor set requirements
  std::map<std::shared_ptr<DescriptorSetRequirer>, std::vector<VkDescriptorSetLayoutBinding>> requirements;
  for (const std::shared_ptr<RenderPass>& renderPass: renderPasses) {
    for (const std::shared_ptr<Pipeline>& pipeline: renderPass->getPipelines() | std::ranges::views::values) {
      pipeline->getMaterial()->computeDescriptorSetRequirements(requirements, renderPass, pipeline);
    }
  }
  std::size_t i = std::numeric_limits<std::size_t>::max();
  std::map<std::shared_ptr<DescriptorSetRequirer>, std::size_t> requirementIndices;
  for (const std::shared_ptr<DescriptorSetRequirer>& requirer: requirements | std::ranges::views::keys) requirementIndices[requirer] = ++i;
  auto perSetDescriptorBindings = requirements | std::ranges::views::values;

  // Create VkDescriptorSetLayout objects
  std::vector<VkDescriptorSetLayout> layouts(perSetDescriptorBindings.size());
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
  };
  i = std::numeric_limits<std::size_t>::max();
  for (const std::vector<VkDescriptorSetLayoutBinding>& bindings: perSetDescriptorBindings) {
    descriptorSetLayoutCreateInfo.bindingCount = bindings.size();
    descriptorSetLayoutCreateInfo.pBindings = bindings.data();
    if (const VkResult result = vkCreateDescriptorSetLayout(device->device, &descriptorSetLayoutCreateInfo, nullptr, &layouts[++i]); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create descriptor set layout");
  }

  /*****************************************
   * Process Materials and Their Pipelines *
   * ***************************************/
  VkDescriptorSetLayout frameDataLayout = requirementIndices.contains(nullptr) ? layouts.at(requirementIndices.at(nullptr)) : VK_NULL_HANDLE;
  {  // Bake pipelines
    std::vector<void*> miscMemoryPool;
    std::vector<VkGraphicsPipelineCreateInfo> pipelineCreateInfos;
    std::vector<VkPipeline*> pipelines;
    std::vector<VkDescriptorSetLayout> setLayouts;
    setLayouts.reserve(3);
    for (const std::shared_ptr<RenderPass>& renderPass : renderPasses)
      for (const std::shared_ptr<Pipeline>& pipeline : renderPass->getPipelines() | std::views::values) {
        setLayouts.clear();
        if (frameDataLayout) setLayouts.emplace_back(frameDataLayout);
        if (requirementIndices.contains(renderPass)) setLayouts.emplace_back(layouts.at(requirementIndices.at(renderPass)));
        if (requirementIndices.contains(pipeline)) setLayouts.emplace_back(layouts.at(requirementIndices.at(pipeline)));
        pipeline->bake(renderPass, 0, setLayouts, miscMemoryPool, pipelineCreateInfos, pipelines);
      }
    std::vector<VkPipeline> tempPipelines(pipelines.size());
    if (!pipelines.empty())
      if (const VkResult result = vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, pipelineCreateInfos.size(), pipelineCreateInfos.data(), nullptr, tempPipelines.data()); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create graphics pipelines");
    for (uint32_t j{}; j < tempPipelines.size(); ++j) *pipelines.at(j) = tempPipelines.at(j);
    for (void* memory: miscMemoryPool) free(memory);
    miscMemoryPool.clear();
    pipelineCreateInfos.clear();
    pipelines.clear();
  }

  /****************************
   * Bake the Command Buffers *
   ****************************/
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
  {  // Allocate descriptor sets
    std::vector<VkDescriptorSetLayout> framesInFlightLayouts;
    framesInFlightLayouts.reserve(layouts.size() * FRAMES_IN_FLIGHT);  // One layout for each descriptor set for each frame in flight
    for (const VkDescriptorSetLayout& layout : layouts) for (int j = 0; j < FRAMES_IN_FLIGHT; ++j) framesInFlightLayouts.push_back(layout);
    std::vector<std::vector<VkDescriptorSetLayoutBinding>> framesInFlightBindings;
    framesInFlightBindings.reserve(layouts.size() * FRAMES_IN_FLIGHT);
    for (const std::vector<VkDescriptorSetLayoutBinding>& bindings : perSetDescriptorBindings) for (int j = 0; j < FRAMES_IN_FLIGHT; ++j) framesInFlightBindings.push_back(bindings);
    descriptorSets = device->descriptorSetAllocator.allocate(framesInFlightBindings, framesInFlightLayouts);
  }

  {  // Assign descriptor sets
    std::shared_ptr<VkDescriptorSetLayout> layout;
    if (frameDataLayout) layout = std::shared_ptr<VkDescriptorSetLayout>(new VkDescriptorSetLayout(frameDataLayout), [this](VkDescriptorSetLayout* layout) {
      vkDestroyDescriptorSetLayout(device->device, *layout, nullptr);
      delete layout;
    });
    std::vector<void*> miscMemoryPool;
    std::vector<VkWriteDescriptorSet> writes;
    for (auto& [descriptorSetRequirer, index]: requirementIndices) {
      auto start = static_cast<decltype(descriptorSets)::difference_type>(index) * FRAMES_IN_FLIGHT + descriptorSets.begin();
      if (descriptorSetRequirer) {
        descriptorSetRequirer->setDescriptorSets({start, start + FRAMES_IN_FLIGHT}, layouts[index]);
        descriptorSetRequirer->writeDescriptorSets(miscMemoryPool, writes, *this);
      }
      else for (uint32_t j{}; j < frames.size(); ++j) {
        frames.at(j).descriptorSet = *(start + j);
        frames.at(j).descriptorSetLayout = layout;
      }
    }
    vkUpdateDescriptorSets(device->device, writes.size(), writes.data(), 0, nullptr);
    for (void* memory: miscMemoryPool) free(memory);
  }

  outOfDate = false;
  return true;
}

VkSemaphore RenderGraph::waitForNextFrameData() const {
  const PerFrameData& frameData = getPerFrameData();
  if (const VkResult result = vkWaitForFences(device->device, 1, &frameData.renderFence, true, UINT64_MAX); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to wait for fence");
  if (const VkResult result = vkResetFences(device->device, 1, &frameData.renderFence); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to reset fence");
  return frameData.frameDataSemaphore;
}

void RenderGraph::update() const {
  for (const std::shared_ptr<Mesh>& mesh : device->meshes)
    mesh->update(*this);
  for (const std::shared_ptr<RenderPass>& renderPass : renderPasses)
    renderPass->update();
  uniformBuffer->update({static_cast<uint32_t>(frameNumber), static_cast<float>(Game::getTime())});
}

void RenderGraph::execute(const std::shared_ptr<Image>& swapchainImage, VkSemaphore semaphore) {
  CommandBuffer commandBuffer;
  for (const std::shared_ptr<RenderPass>& renderPass: renderPasses) renderPass->execute(commandBuffer);
  commandBuffer.record<CommandBuffer::BlitImageToImage>(getImage(getImageId(RenderColor)).image, swapchainImage);
  std::vector<VkImageMemoryBarrier> imageBarriers = {
    {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext               = nullptr,
      .srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
      .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
      .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image               = swapchainImage->getImage(),
      .subresourceRange    = swapchainImage->getWholeRange()
    }
  };
  commandBuffer.record<CommandBuffer::PipelineBarrier>(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, std::vector<VkMemoryBarrier>{}, std::vector<VkBufferMemoryBarrier>{}, imageBarriers);
  commandBuffer.preprocess();

  const PerFrameData& frameData = getPerFrameData();
  constexpr VkCommandBufferBeginInfo beginInfo {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext            = nullptr,
      .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr
  };
  vkBeginCommandBuffer(frameData.commandBuffer, &beginInfo);
  commandBuffer.bake(frameData.commandBuffer);
  vkEndCommandBuffer(frameData.commandBuffer);
  constexpr std::array<VkPipelineStageFlags, 1> stageMasks{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
  const VkSubmitInfo submitInfo {
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext                = nullptr,
      .waitSemaphoreCount   = 1,
      .pWaitSemaphores      = &frameData.frameDataSemaphore,
      .pWaitDstStageMask    = stageMasks.data(),
      .commandBufferCount   = 1,
      .pCommandBuffers      = &frameData.commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores    = &semaphore
  };
  if (const VkResult result = vkQueueSubmit(device->globalQueue, 1, &submitInfo, frameData.renderFence); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to submit recorded command buffer to queue");
  ++frameNumber;
}

void RenderGraph::buildImages(const std::unordered_map<ImageID, VkImageUsageFlags>& usages) {
  for (auto& [id, properties]: images) {
    auto& resolutionGroup = resolutionGroups[properties.resolutionGroup];
    properties.image = std::make_shared<Image>(device, properties.name, properties.format, resolutionGroup.resolution, usages.at(id), 1, resolutionGroup.sampleCount);
  }
}

const RenderGraph::PerFrameData& RenderGraph::getPerFrameData(const uint64_t frameIndex) const { return frames[frameIndex == static_cast<decltype(frameIndex)>(-1) ? getFrameIndex() : frameIndex]; }