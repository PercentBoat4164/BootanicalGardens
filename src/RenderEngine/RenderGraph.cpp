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

#include <volk/volk.h>

#include <ranges>
#include <vector>

RenderGraph::PerFrameData::PerFrameData(const std::shared_ptr<GraphicsDevice>& device, const RenderGraph& graph) : device(device), graph(graph) {
  /****************************************
   * Initialize Command Recording Objects *
   ****************************************/
  const VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext            = nullptr,
      .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = device->globalQueueFamilyIndex
  };
  if (const VkResult result = vkCreateCommandPool(device->device, &commandPoolCreateInfo, nullptr, &commandPool); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create command pool.");
  const VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext              = nullptr,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1
  };
  if (const VkResult result = vkAllocateCommandBuffers(device->device, &commandBufferAllocateInfo, &commandBuffer); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to allocate command buffers.");

  /*****************************************
   * Initialize Synchronization Objections *
   *****************************************/
  constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0
  };
  if (const VkResult result = vkCreateSemaphore(device->device, &semaphoreCreateInfo, nullptr, &frameDataSemaphore); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create semaphore.");
  if (const VkResult result = vkCreateSemaphore(device->device, &semaphoreCreateInfo, nullptr, &frameFinishedSemaphore); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create semaphore.");
  constexpr VkFenceCreateInfo fenceCreateInfo {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
  };
  if (const VkResult result = vkCreateFence(device->device, &fenceCreateInfo, nullptr, &renderFence); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create fence");
}

RenderGraph::PerFrameData::~PerFrameData() {
  if (commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(device->device, commandPool, nullptr);
  commandPool = VK_NULL_HANDLE;
  vkQueueWaitIdle(device->globalQueue);
  if (frameFinishedSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device->device, frameFinishedSemaphore, nullptr);
  frameFinishedSemaphore = VK_NULL_HANDLE;
  if (frameDataSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device->device, frameDataSemaphore, nullptr);
  frameDataSemaphore = VK_NULL_HANDLE;
  if (renderFence != VK_NULL_HANDLE) vkDestroyFence(device->device, renderFence, nullptr);
  renderFence = VK_NULL_HANDLE;
}

uint8_t RenderGraph::FRAMES_IN_FLIGHT = 2;

RenderGraph::RenderGraph(const std::shared_ptr<GraphicsDevice>& device) : device(device) {
  // Build frame data
  const std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets = device->perFrameDescriptorAllocator.allocate(FRAMES_IN_FLIGHT);
  frames.reserve(FRAMES_IN_FLIGHT);
  for (uint32_t i{}; i < FRAMES_IN_FLIGHT; ++i) {
    frames.emplace_back(device, *this);
    frames.back().descriptorSet = descriptorSets[i];
  }

  std::array layouts {
    device->perFrameDescriptorAllocator.getLayout(),
    device->perPassDescriptorAllocator.getLayout(),
    device->perMaterialDescriptorAllocator.getLayout(),
    device->perMeshDescriptorAllocator.getLayout()
  };

  const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext                  = nullptr,
    .flags                  = 0,
    .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
    .pSetLayouts            = layouts.data(),
    .pushConstantRangeCount = 0,
    .pPushConstantRanges    = nullptr
  };
  if (const VkResult result = vkCreatePipelineLayout(device->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create pipeline layout.");

  uniformBuffer = std::make_shared<UniformBuffer<GraphData>>(device, "RenderGraph UniformBuffer");

  const VkDescriptorBufferInfo bufferInfo {
    .buffer = uniformBuffer->getBuffer(),
    .offset = 0,
    .range = VK_WHOLE_SIZE
  };
  std::vector<VkWriteDescriptorSet> writeDescriptorSet(frames.size(), {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr,
    .dstSet = VK_NULL_HANDLE,
    .dstBinding = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .pImageInfo = VK_NULL_HANDLE,
    .pBufferInfo = &bufferInfo,
    .pTexelBufferView = VK_NULL_HANDLE
  });
  for (uint32_t i{}; i < frames.size(); ++i) writeDescriptorSet[i].dstSet = *frames[i].descriptorSet;
  vkUpdateDescriptorSets(device->device, writeDescriptorSet.size(), writeDescriptorSet.data(), 0, nullptr);
}

RenderGraph::~RenderGraph() {
  const PerFrameData& frameData = getPerFrameData();
  if (const VkResult result = vkWaitForFences(device->device, 1, &frameData.renderFence, VK_TRUE, UINT64_MAX); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to wait for fence.");
  vkDestroyPipelineLayout(device->device, pipelineLayout, nullptr);
  pipelineLayout = VK_NULL_HANDLE;
}

void RenderGraph::setResolutionGroup(const ResolutionGroupID id, const VkExtent3D resolution) {
  auto& group = resolutionGroups[id];
  std::get<0>(group) = resolution;
  for (AttachmentID attachmentId : std::get<1>(group)) if (std::shared_ptr<Image> image = backingImages[attachmentId]; image != nullptr) image->rebuild(resolution);
  outOfDate = true;
}

void RenderGraph::setAttachment(const AttachmentID id, const ResolutionGroupID groupId, const VkFormat format, const VkSampleCountFlags sampleCount) {
  attachmentsProperties[id] = {groupId, format, sampleCount};
  std::get<1>(resolutionGroups[groupId]).push_back(id);
  outOfDate = true;
}

void RenderGraph::addRenderable(const std::shared_ptr<Renderable>& renderable) {
  renderables.emplace(renderable);
  for (const std::shared_ptr<Mesh>& mesh : renderable->getMeshes())
    for (const std::shared_ptr<RenderPass>& renderPass: renderPasses)
      if ((mesh->isOpaque() && renderPass->meshFilter & RenderPass::OpaqueBit) || (mesh->isTransparent() && renderPass->meshFilter & RenderPass::TransparentBit))
        renderPass->addMesh(mesh);
}

void RenderGraph::removeRenderable(const std::shared_ptr<Renderable>& renderable) {
  renderables.erase(renderables.get_iterator(&renderable));
  for (const std::shared_ptr<Mesh>& mesh : renderable->getMeshes())
    for (const std::shared_ptr<RenderPass>& renderPass: renderPasses)
      if ((mesh->isOpaque() && renderPass->meshFilter & RenderPass::OpaqueBit) || (mesh->isTransparent() && renderPass->meshFilter & RenderPass::TransparentBit))
        renderPass->removeMesh(mesh);
}

bool RenderGraph::bake(CommandBuffer& commandBuffer) {
  if (!outOfDate) return true;
  // Understand how attachments are used across render passes
  std::unordered_map<RenderPass*, std::vector<AttachmentID>> pass2id;
  std::unordered_map<AttachmentID, std::vector<std::pair<RenderPass*, AttachmentDeclaration>>> id2decl;
  std::unordered_map<AttachmentID, AttachmentDeclaration> declarations;
  for (const std::shared_ptr<RenderPass>& renderPass : renderPasses) {
    auto pairs = renderPass->declareAttachments();
    auto range = std::ranges::views::keys(pairs);
    pass2id[renderPass.get()] = std::vector<AttachmentID>{range.begin(), range.end()};
    for (auto& [id, declaration] : pairs) {
      declarations[id] = declaration;
      id2decl[id].emplace_back(renderPass.get(), declaration);
    }
  }
  buildImages();
  // Utilize that knowledge to bake each render pass
  for (const std::shared_ptr<RenderPass>& renderPass : renderPasses) {
    const std::vector<AttachmentID>& renderPassAttachmentIDs = pass2id.at(renderPass.get());
    std::vector<VkAttachmentDescription> descriptions;
    descriptions.reserve(renderPassAttachmentIDs.size());
    std::vector<std::shared_ptr<Image>> images;
    images.reserve(renderPassAttachmentIDs.size());
    for (const AttachmentID& id: renderPassAttachmentIDs) {
      /**@todo: Add support for aliasing attachmentsProperties.*/
      std::shared_ptr<Image> image = backingImages.at(id);
      auto& attachmentDeclarations = id2decl[id];
      // Find this renderpass in the declarations of this attachment.
      const auto thisIt = std::ranges::find(attachmentDeclarations, renderPass.get(), &decltype(id2decl)::mapped_type::value_type::first);
      // Find the next renderpass in the declarations of this attachment.
      auto nextIt = thisIt;
      if (++nextIt == attachmentDeclarations.end()) nextIt = attachmentDeclarations.begin();
      /**@todo: Optimize load and store ops.*/
      /**@todo: Log an error if the format does not include a stencil buffer, but the stencilLoadOp or stencilStoreOp are not DONT_CARE.*/
      const AttachmentDeclaration& thisDeclaration = thisIt->second;
      const AttachmentDeclaration& nextDeclaration = nextIt->second;
      descriptions.push_back({
        .flags = 0U,
        .format = image->getFormat(),
        .samples = image->getSampleCount(),
        .loadOp = nextDeclaration.loadOp,
        .storeOp = nextDeclaration.storeOp,
        .stencilLoadOp = nextDeclaration.stencilLoadOp,
        .stencilStoreOp = nextDeclaration.stencilStoreOp,
        .initialLayout = thisDeclaration.layout,
        .finalLayout = nextDeclaration.layout
      });
      images.push_back(image);
    }
    renderPass->bake(descriptions, images);
  }
  transitionImages(commandBuffer, declarations);
  
  // Allocate per frame and per render pass descriptor sets
  outOfDate = false;
  return true;
}

VkSemaphore RenderGraph::waitForNextFrameData() const {
  const PerFrameData& frameData = getPerFrameData();
  if (const VkResult result = vkWaitForFences(device->device, 1, &frameData.renderFence, true, UINT64_MAX); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to wait for fence.");
  if (const VkResult result = vkResetFences(device->device, 1, &frameData.renderFence); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to reset fence.");
  return frameData.frameDataSemaphore;
}

void RenderGraph::update() const {
  for (const std::shared_ptr<Renderable>& renderable : renderables)
    for (const std::shared_ptr<Mesh>& mesh : renderable->getMeshes())
      mesh->update(*this);
  for (const std::shared_ptr<Pipeline>& pipeline : std::ranges::views::values(pipelines))
    pipeline->update();
  uniformBuffer->update({frameNumber, Game::getTime()});
}

VkSemaphore RenderGraph::execute(const std::shared_ptr<Image>& swapchainImage) const {
  CommandBuffer commandBuffer;
  std::shared_ptr<Image> defaultColorImage = backingImages.at(RenderColor);
  // commandBuffer.record<CommandBuffer::ClearColorImage>(swapchainImage);
  commandBuffer.record<CommandBuffer::ClearColorImage>(defaultColorImage);
  commandBuffer.record<CommandBuffer::ClearDepthStencilImage>(backingImages.at(RenderDepth));
  VkImageMemoryBarrier imageMemoryBarrier {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .pNext = nullptr,
    .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
    .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .srcQueueFamilyIndex = device->globalQueueFamilyIndex,
    .dstQueueFamilyIndex = device->globalQueueFamilyIndex,
    .image = defaultColorImage->getImage(),
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };
  commandBuffer.record<CommandBuffer::PipelineBarrier>(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, std::vector<VkMemoryBarrier>{}, std::vector<VkBufferMemoryBarrier>{}, std::vector{imageMemoryBarrier});
  for (const std::shared_ptr<RenderPass>& renderPass: renderPasses) {
    renderPass->update(*this);
    renderPass->execute(commandBuffer);
  }
  commandBuffer.record<CommandBuffer::BlitImageToImage>(defaultColorImage, swapchainImage);
  imageMemoryBarrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .pNext = nullptr,
    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .srcQueueFamilyIndex = device->globalQueueFamilyIndex,
    .dstQueueFamilyIndex = device->globalQueueFamilyIndex,
    .image = swapchainImage->getImage(),
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };
  commandBuffer.record<CommandBuffer::PipelineBarrier>(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, std::vector<VkMemoryBarrier>{}, std::vector<VkBufferMemoryBarrier>{}, std::vector{imageMemoryBarrier});
  // commandBuffer.preprocess({.resourceStates = {{static_cast<Resource*>(swapchainImage.get())->getObject(), CommandBuffer::ResourceState{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_READ_BIT}}}});
  // @todo: Providing state to a CommandBuffer as seen above is causes preprocessing to fail. Fix this.
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
  std::array<VkPipelineStageFlags, 1> stageMasks{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
  const VkSubmitInfo submitInfo {
      .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext                = nullptr,
      .waitSemaphoreCount   = 1,
      .pWaitSemaphores      = &frameData.frameDataSemaphore,
      .pWaitDstStageMask    = stageMasks.data(),
      .commandBufferCount   = 1,
      .pCommandBuffers      = &frameData.commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores    = &frameData.frameFinishedSemaphore
  };
  if (const VkResult result = vkQueueSubmit(device->globalQueue, 1, &submitInfo, frameData.renderFence); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to submit recorded commandbuffer to queue.");
  return frameData.frameFinishedSemaphore;
}

/**@todo Do not tie Pipelines to specific Materials so that they can be reused more effectively.*/
void RenderGraph::bakePipeline(const std::shared_ptr<const Material>& material, const std::shared_ptr<const RenderPass>& renderPass) {
  pipelines[reinterpret_cast<uint64_t>(material.get()) ^ renderPass->compatibility] = std::make_shared<Pipeline>(device, material, renderPass, pipelineLayout);
}

std::shared_ptr<Pipeline> RenderGraph::getPipeline(const std::shared_ptr<const Material>& material, const uint64_t renderPassCompatibility) {
  return pipelines[reinterpret_cast<uint64_t>(material.get()) ^ renderPassCompatibility];
}

void RenderGraph::buildImages() {
  backingImages.clear();
  for (const auto& [id, properties]: attachmentsProperties) {
    /**@todo: Generate the correct image usage flags for each image based on how it is used.*/
    backingImages[id] = std::make_shared<Image>(device, "", properties.format, std::get<0>(resolutionGroups[properties.resolutionGroup]), properties.format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  }
}

void RenderGraph::transitionImages(CommandBuffer& commandBuffer, const std::unordered_map<AttachmentID, AttachmentDeclaration>& declarations) {
  for (auto& [id, image] : backingImages) {
    const AttachmentDeclaration& declaration = declarations.at(id);
    const VkImageMemoryBarrier imageMemoryBarrier {
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
    };
    commandBuffer.record<CommandBuffer::PipelineBarrier>(VK_PIPELINE_STAGE_TRANSFER_BIT, declaration.stage, 0, std::vector<VkMemoryBarrier>{}, std::vector<VkBufferMemoryBarrier>{}, std::vector{imageMemoryBarrier});
  }
}

const RenderGraph::PerFrameData& RenderGraph::getPerFrameData() const { return frames[getFrameIndex()]; }
VkPipelineLayout RenderGraph::getPipelineLayout() const { return pipelineLayout; }