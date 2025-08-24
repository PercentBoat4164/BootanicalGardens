#include "RenderGraph.hpp"

#include "src/Game/Game.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Mesh.hpp"
#include "src/RenderEngine/Renderable/Renderable.hpp"
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

void RenderGraph::setResolutionGroup(const ResolutionGroupID id, const VkExtent3D resolution) {
  auto& group = resolutionGroups[id];
  std::get<0>(group) = resolution;
  for (AttachmentID attachmentId : std::get<1>(group)) if (std::shared_ptr<Image> image = backingImages[attachmentId]; image != nullptr) image->rebuild(resolution);
  outOfDate = true;
}

void RenderGraph::setAttachment(const AttachmentID id, const std::string_view& name, const ResolutionGroupID groupId, const VkFormat format, const VkSampleCountFlags sampleCount) {
  if (!name.empty()) attachmentNames[id] = name;
  attachmentsProperties[id] = {groupId, format, sampleCount};
  std::get<1>(resolutionGroups[groupId]).push_back(id);
  outOfDate = true;
}

std::shared_ptr<Image> RenderGraph::getAttachmentImage(const AttachmentID id) const {
  return backingImages.at(id);
}

/**
 * Guarantees that the RenderGraph is baked. If the RenderGraph is not out-of-date, then no baking will actually occur.
 *
 * @todo: Needs heavy optimization. This function's high speed will be crucial for avoiding hitches or objects popping in. This function should be heavily threaded.
 *
 * @param commandBuffer A scratch <c>CommandBuffer</c> that setup commands will be recorded into.
 * @return <c>true</c> if baking actually happened, <c>false</c> otherwise.
 */
bool RenderGraph::bake(CommandBuffer& commandBuffer) {
  if (!outOfDate) return false;
  /*************************
   * Process Render Passes *
   *************************/
  {
    // Understand how attachments are used across RenderPasses
    std::unordered_map<RenderPass*, std::vector<AttachmentID>> pass2id;
    std::unordered_map<AttachmentID, std::vector<std::pair<RenderPass*, AttachmentDeclaration>>> id2decl;
    std::unordered_map<AttachmentID, AttachmentDeclaration> declarations;
    std::unordered_map<AttachmentID, VkImageUsageFlags> usages {
      {GBufferAlbedo, VK_IMAGE_USAGE_TRANSFER_SRC_BIT},
      {GBufferDepth, VK_IMAGE_USAGE_SAMPLED_BIT},
      {GBufferPosition, VK_IMAGE_USAGE_SAMPLED_BIT},
      {getAttachmentId("ShadowDepth"), VK_IMAGE_USAGE_SAMPLED_BIT}};
    for (const std::shared_ptr<RenderPass>& renderPass : renderPasses) {
      std::vector<std::pair<AttachmentID, AttachmentDeclaration>> pairs = renderPass->declareAttachments();
      auto range = std::views::keys(pairs);
      pass2id[renderPass.get()] = std::vector<AttachmentID>{range.begin(), range.end()};
      for (auto& [id, declaration] : pairs) {
        /**@todo: Log the name of the render pass and the attachment.*/
        if (!attachmentsProperties.contains(id)) GraphicsInstance::showError("Unregistered AttachmentID!");
        declarations[id] = declaration;
        usages[id] |= declaration.usage;
        id2decl[id].emplace_back(renderPass.get(), declaration);
      }
    }
    buildImages(usages);

    // Bake RenderPasses
    for (const std::shared_ptr<RenderPass>& renderPass : renderPasses) {
      const std::vector<AttachmentID>& renderPassAttachmentIDs = pass2id.at(renderPass.get());
      std::vector<VkAttachmentDescription> descriptions;
      descriptions.reserve(renderPassAttachmentIDs.size());
      std::vector<std::shared_ptr<Image>> images;
      images.reserve(renderPassAttachmentIDs.size());
      for (const AttachmentID& id: renderPassAttachmentIDs) {
        /**@todo: Add support for aliasing attachmentsProperties.*/
        /**@todo: Add support for reordering render passes.*/
        std::shared_ptr<Image> image = backingImages.at(id);
        auto& attachmentDeclarations = id2decl.at(id);
        // Find this renderpass in the declarations of this attachment.
        const auto thisIt = std::ranges::find(attachmentDeclarations, renderPass.get(), &decltype(id2decl)::mapped_type::value_type::first);
        // Find the next renderpass in the declarations of this attachment.
        auto nextIt = thisIt;
        if (++nextIt == attachmentDeclarations.end()) nextIt = attachmentDeclarations.begin();
        /**@todo: Optimize load and store ops.*/
        /**@todo: Log an error if the format does not include a stencil buffer, but the stencilLoadOp or stencilStoreOp are not DONT_CARE.*/
        /**@todo: Log an error if this the loadOp is CLEAR and the attachment is an input attachment.*/
        const AttachmentDeclaration& thisDeclaration = thisIt->second;
        const AttachmentDeclaration& nextDeclaration = nextIt->second;
        // VkAttachmentLoadOp loadOp = thisDeclaration.loadOp;
        // if (loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE) {
        //   if (nextDeclaration)
        // }
        descriptions.push_back({
            .flags = 0U,
            .format = image->getFormat(),
            .samples = image->getSampleCount(),
            .loadOp = thisDeclaration.loadOp,
            .storeOp = nextDeclaration.storeOp,
            .stencilLoadOp = thisDeclaration.stencilLoadOp,
            .stencilStoreOp = nextDeclaration.stencilStoreOp,
            .initialLayout = thisDeclaration.layout,
            .finalLayout = nextDeclaration.layout
        });
        images.push_back(image);
      }
      renderPass->bake(descriptions, images);
      if (renderPass->compatibility == -1U) GraphicsInstance::showError("`RenderPass::bake(...)` failed to update the RenderPass compatibility.");
    }
    transitionImages(commandBuffer, declarations);
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
  std::shared_ptr<Image> defaultColorImage = backingImages.at(GBufferAlbedo);
  for (const std::shared_ptr<RenderPass>& renderPass: renderPasses) renderPass->execute(commandBuffer);
  commandBuffer.record<CommandBuffer::BlitImageToImage>(defaultColorImage, swapchainImage);
  std::vector<VkImageMemoryBarrier> imageBarriers{
    {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext               = nullptr,
      .srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
      .dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT,
      .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = device->globalQueueFamilyIndex,
      .dstQueueFamilyIndex = device->globalQueueFamilyIndex,
      .image               = swapchainImage->getImage(),
      .subresourceRange    = swapchainImage->getWholeRange()
    },
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

void RenderGraph::buildImages(const std::unordered_map<AttachmentID, VkImageUsageFlags>& usages) {
  backingImages.clear();
  for (const auto& [id, properties]: attachmentsProperties) {
    backingImages[id] = std::make_shared<Image>(device, attachmentNames.at(id), properties.format, std::get<0>(resolutionGroups[properties.resolutionGroup]), usages.at(id));
  }
}

void RenderGraph::transitionImages(CommandBuffer& commandBuffer, const std::unordered_map<AttachmentID, AttachmentDeclaration>& declarations) {
  std::map<VkPipelineStageFlags, std::vector<VkImageMemoryBarrier>> imageBarrierGroups;
  for (auto& [id, image] : backingImages) {
    const AttachmentDeclaration& declaration = declarations.at(id);
    imageBarrierGroups[declaration.stage].push_back(VkImageMemoryBarrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .pNext = nullptr,
      .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
      .dstAccessMask = declaration.access,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = declaration.layout,
      .srcQueueFamilyIndex = device->globalQueueFamilyIndex,
      .dstQueueFamilyIndex = device->globalQueueFamilyIndex,
      .image = image->getImage(),
      .subresourceRange = image->getWholeRange()
    });
  }
  for (const auto& [stage, imageBarriers] : imageBarrierGroups)
    commandBuffer.record<CommandBuffer::PipelineBarrier>(VK_PIPELINE_STAGE_TRANSFER_BIT, stage, 0, std::vector<VkMemoryBarrier>{}, std::vector<VkBufferMemoryBarrier>{}, imageBarriers);
}

const RenderGraph::PerFrameData& RenderGraph::getPerFrameData(const uint64_t frameIndex) const { return frames[frameIndex == static_cast<decltype(frameIndex)>(-1) ? getFrameIndex() : frameIndex]; }