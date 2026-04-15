#include "CommandBuffer.hpp"

#include "src/RenderEngine/Pipeline/Pipeline.hpp"
#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/Resources/Resource.hpp"
#include "src/RenderEngine/Framebuffer.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/MeshGroup/Vertex.hpp"
#include "src/Tools/Vulkan/Access.h"

#include <volk/volk.h>

#include <algorithm>
#include <utility>

CommandBuffer::Command::Command(std::vector<ResourceAccess> accesses, const Type type) : accesses(std::move(accesses)), type(type)
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
, trace(cpptrace::generate_raw_trace(4))  // Skipping 4 layers brings us to either the first of two calls to `record`, or the place that record was initially called.
#endif
{}

void CommandBuffer::BeginRenderPass::preprocess(State& state, PreprocessingFlags flags) {
  renderPassBeginInfo = {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .pNext           = nullptr,
    .renderPass      = renderPass->getRenderPass(),
    .framebuffer     = renderPass->getFramebuffer()->getFramebuffer(),
    .renderArea      = renderArea,
    .clearValueCount = static_cast<uint32_t>(clearValues.size()),
    .pClearValues    = clearValues.data()
  };

  state.renderPass = renderPass;
}
void CommandBuffer::BeginRenderPass::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BeginRenderPass::toString(const bool includeArguments) {
  if (!includeArguments) return "vkCmdBeginRenderPass";
  std::string string = "vkCmdBeginRenderPass\n"
  "\tRender Pass: " + std::to_string(reinterpret_cast<uint64_t>(renderPass)) + "\n"
  "\tRender Area:\n"
  "\t\textent: " + std::to_string(renderArea.extent.width) + "x" + std::to_string(renderArea.extent.height) + "\n"
  "\t\toffset: " + std::to_string(renderArea.offset.x) + ", " + std::to_string(renderArea.offset.y) + "";
  if (!clearValues.empty()) {
    string += "\n\tClear Values (" + std::to_string(clearValues.size()) + "):";
    uint64_t index{};
    for (const VkClearValue& value: clearValues) {
      string += "\n\t\t#" + std::to_string(index++) + ":\n"
      "\t\t\tcolor: [" + std::to_string(value.color.float32[0]) + ", " + std::to_string(value.color.float32[1]) + ", " + std::to_string(value.color.float32[2]) + ", " + std::to_string(value.color.float32[3]) + "]\n"
      "\t\t\tdepth: [" + std::to_string(value.depthStencil.depth) + ", " + std::to_string(value.depthStencil.stencil) + "]";
    }
  }
  return string;
}

void CommandBuffer::BindDescriptorSets::preprocess(State& state, PreprocessingFlags flags) {
  pipelineLayout = state.pipeline->getLayout();
}
void CommandBuffer::BindDescriptorSets::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, firstSet, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindDescriptorSets::toString(bool includeArguments) {
  return "vkCmdBindDescriptorSets";
}

CommandBuffer::BindIndexBuffer::BindIndexBuffer(const Buffer* const indexBuffer) :
    Command({}, StateChange),
    buffer(indexBuffer) {}
void CommandBuffer::BindIndexBuffer::preprocess(State& state, PreprocessingFlags flags) {
  state.indexBuffer = buffer;
}
void CommandBuffer::BindIndexBuffer::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBindIndexBuffer(commandBuffer, buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32); /**@todo: Expose these options.*/
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindIndexBuffer::toString(bool includeArguments) {
  return "vkCmdBindIndexBuffer";
}

CommandBuffer::BindPipeline::BindPipeline(const Pipeline* const pipeline) :
    Command({}, StateChange),
    pipeline(pipeline),
    extent{} {}
void CommandBuffer::BindPipeline::preprocess(State& state, PreprocessingFlags flags) {
  state.pipeline = pipeline;
  if (state.renderPass == nullptr) GraphicsInstance::showError("BeginRenderPass must be called before BindPipeline");
  extent = state.renderPass->getFramebuffer()->getExtent();
}
void CommandBuffer::BindPipeline::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  /**@todo: Handle dynamic state in other commands.*/
  const VkViewport viewport{0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  const VkRect2D rect{0, 0, extent.width, extent.height};
  vkCmdSetScissor(commandBuffer, 0, 1, &rect);
  vkCmdBindPipeline(commandBuffer, pipeline->bindPoint, pipeline->getPipeline());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindPipeline::toString(bool includeArguments) {
  return "vkCmdBindPipeline";
}

void CommandBuffer::BindVertexBuffers::preprocess(State& state, PreprocessingFlags flags) {
  if (buffers.size() != offsets.size()) GraphicsInstance::showError("buffers and offsets must be the same size");
  state.vertexBuffers.resize(std::max(state.vertexBuffers.size(), buffers.size() + firstBinding));
  for (uint32_t i = 0; i < buffers.size(); ++i) state.vertexBuffers[i + firstBinding] = buffers[i];
}
void CommandBuffer::BindVertexBuffers::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  auto* rawBuffers = static_cast<VkBuffer*>(alloca(buffers.size() * sizeof(VkBuffer)));
  for (size_t i = 0; i < buffers.size(); i++) rawBuffers[i] = buffers[i] ? buffers[i]->getBuffer() : VK_NULL_HANDLE;
  vkCmdBindVertexBuffers(commandBuffer, firstBinding, buffers.size(), rawBuffers, offsets.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindVertexBuffers::toString(bool includeArguments) {
  return "vkCmdBindVertexBuffers";
}

void CommandBuffer::BlitImageToImage::preprocess(State& state, PreprocessingFlags flags) {
  srcImageLayout = state.resourceStates.at(src).layout;
  dstImageLayout = state.resourceStates.at(dst).layout;
}
void CommandBuffer::BlitImageToImage::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBlitImage(commandBuffer, src->getImage(), srcImageLayout, dst->getImage(), dstImageLayout, blits.size(), blits.data(), filter);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BlitImageToImage::toString(bool includeArguments) {
  return "vkCmdBlitImage";
}

void CommandBuffer::ClearColorImage::preprocess(State& state, PreprocessingFlags flags) {
  layout = state.resourceStates.at(image).layout;
}
void CommandBuffer::ClearColorImage::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdClearColorImage(commandBuffer, image->getImage(), layout, &value, ranges.size(), ranges.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::ClearColorImage::toString(bool includeArguments) {
  return "vkCmdClearColorImage";
}

void CommandBuffer::ClearDepthStencilImage::preprocess(State& state, PreprocessingFlags flags) {
  layout = state.resourceStates.at(image).layout;
}
void CommandBuffer::ClearDepthStencilImage::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdClearDepthStencilImage(commandBuffer, image->getImage(), layout, &value, ranges.size(), ranges.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::ClearDepthStencilImage::toString(bool includeArguments) {
  return "vkCmdClearColorImage";
}

void CommandBuffer::CopyBufferToBuffer::preprocess(State& state, PreprocessingFlags flags) {}
void CommandBuffer::CopyBufferToBuffer::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyBuffer(commandBuffer, src->getBuffer(), dst->getBuffer(), copies.size(), copies.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyBufferToBuffer::toString(bool includeArguments) {
  return "vkCmdCopyBuffer";
}

void CommandBuffer::CopyBufferToImage::preprocess(State& state, PreprocessingFlags flags) {
  layout = state.resourceStates.at(dst).layout;
}
void CommandBuffer::CopyBufferToImage::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyBufferToImage(commandBuffer, src->getBuffer(), dst->getImage(), layout, copies.size(), copies.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyBufferToImage::toString(bool includeArguments) {
  return "vkCmdCopyBufferToImage";
}

void CommandBuffer::CopyImageToBuffer::preprocess(State& state, PreprocessingFlags flags) {
  layout = state.resourceStates.at(src).layout;
}
void CommandBuffer::CopyImageToBuffer::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyImageToBuffer(commandBuffer, src->getImage(), layout, dst->getBuffer(), copies.size(), copies.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyImageToBuffer::toString(bool includeArguments) {
  return "vkCmdCopyImageToBuffer";
}

void CommandBuffer::CopyImageToImage::preprocess(State& state, PreprocessingFlags flags) {
  srcLayout = state.resourceStates.at(src).layout;
  dstLayout = state.resourceStates.at(dst).layout;
}
void CommandBuffer::CopyImageToImage::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyImage(commandBuffer, src->getImage(), srcLayout, dst->getImage(), dstLayout, copies.size(), copies.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyImageToImage::toString(bool includeArguments) {
  return "vkCmdCopyImage";
}

CommandBuffer::Draw::Draw(const uint32_t vertexCount) : Command({}, Command::Draw), vertexCount(vertexCount) {}
void CommandBuffer::Draw::preprocess(State& state, PreprocessingFlags flags) {
  if (state.renderPass == nullptr) GraphicsInstance::showError("must call BeginRenderPass before Draw");
  if (state.pipeline == nullptr) GraphicsInstance::showError("must call BindPipeline before Draw");
  if (vertexCount != 0) return;
  if (state.vertexBuffers.empty()) GraphicsInstance::showError("must call BindVertexBuffers before Draw, or set vertexCount != 0");
}
void CommandBuffer::Draw::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::Draw::toString(bool includeArguments) {
  return "vkCmdDraw";
}

CommandBuffer::DrawIndexed::DrawIndexed(const uint32_t instanceCount) : Command({}, Draw), instanceCount(instanceCount) {}
void CommandBuffer::DrawIndexed::preprocess(State& state, PreprocessingFlags flags) {
  if (state.renderPass == nullptr) GraphicsInstance::showError("must call BeginRenderPass before DrawIndexed");
  if (state.pipeline == nullptr) GraphicsInstance::showError("must call BindPipeline before DrawIndexed");
  if (state.vertexBuffers.empty()) GraphicsInstance::showError("must call BindIndexBuffer before DrawIndexed");
  indexCount = state.indexBuffer->getSize() / sizeof(uint32_t);
  if (state.vertexBuffers.empty() || state.vertexBuffers.at(0) == nullptr) GraphicsInstance::showError("must call BindVertexBuffers before DrawIndexed");
}
void CommandBuffer::DrawIndexed::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::DrawIndexed::toString(bool includeArguments) {
  return "vkCmdDrawIndexed";
}

CommandBuffer::DrawIndexedIndirect::DrawIndexedIndirect(const Buffer* const buffer, const uint32_t count, const VkDeviceSize offset, const uint32_t stride) : Command({}, Draw), buffer(buffer), offset(offset), count(count), stride(stride) {
  if (count == std::numeric_limits<uint32_t>::max()) this->count = buffer->getSize() / stride;
}
void CommandBuffer::DrawIndexedIndirect::preprocess(State& state, PreprocessingFlags flags) {
  if (state.renderPass == nullptr) GraphicsInstance::showError("must call BeginRenderPass before DrawIndexedIndirect");
  if (state.pipeline == nullptr) GraphicsInstance::showError("must call BindPipeline before DrawIndexedIndirect");
}
void CommandBuffer::DrawIndexedIndirect::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdDrawIndexedIndirect(commandBuffer, buffer->getBuffer(), offset, count, stride);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::DrawIndexedIndirect::toString(bool includeArguments) {
  return "vkCmdDrawIndexedIndirect";
}

CommandBuffer::EndRenderPass::EndRenderPass() : Command({}, StateChange) {}
void CommandBuffer::EndRenderPass::preprocess(State& state, PreprocessingFlags flags) {
  for (const auto& [image, layout]: state.renderPass->getImageLayoutsAfterExecution()) {
    ResourceState& resourceState = state.resourceStates[image];
    resourceState.layout = layout;
    resourceState.access = nullptr;
    resourceState.stage = nullptr;
  }

  state.renderPass = nullptr;
  state.pipeline = nullptr;
}
void CommandBuffer::EndRenderPass::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdEndRenderPass(commandBuffer);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::EndRenderPass::toString(bool includeArguments) {
  return "vkCmdEndRenderPass";
}

void CommandBuffer::PipelineBarrier::preprocess(State& state, const PreprocessingFlags flags) {
  for (auto& imageMemoryBarrier: imageMemoryBarriers) {
    assert(imageMemoryBarrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);
  }
}
void CommandBuffer::PipelineBarrier::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  auto* memoryBarrierData       = static_cast<VkMemoryBarrier*>(      alloca(memoryBarriers.size()       * sizeof(VkMemoryBarrier)      ));
  auto* bufferMemoryBarrierData = static_cast<VkBufferMemoryBarrier*>(alloca(bufferMemoryBarriers.size() * sizeof(VkBufferMemoryBarrier)));
  auto* imageMemoryBarrierData  = static_cast<VkImageMemoryBarrier*>( alloca(imageMemoryBarriers.size()  * sizeof(VkImageMemoryBarrier) ));
  size_t i = std::numeric_limits<size_t>::max();
  for (const MemoryBarrier& barrier : memoryBarriers) memoryBarrierData [++i] = static_cast<VkMemoryBarrier>(barrier);
  i = std::numeric_limits<size_t>::max();
  for (const BufferMemoryBarrier& barrier : bufferMemoryBarriers) bufferMemoryBarrierData[++i] = static_cast<VkBufferMemoryBarrier>(barrier);
  i = std::numeric_limits<size_t>::max();
  for (const ImageMemoryBarrier& barrier : imageMemoryBarriers) imageMemoryBarrierData [++i] = static_cast<VkImageMemoryBarrier>(barrier);
  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarriers.size(), memoryBarrierData, bufferMemoryBarriers.size(), bufferMemoryBarrierData, imageMemoryBarriers.size(), imageMemoryBarrierData);
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::PipelineBarrier::toString(bool includeArguments) {
  return "vkCmdPipelineBarrier";
}

void CommandBuffer::PushConstants::preprocess(State& state, PreprocessingFlags flags) {
  layout = state.pipeline->getLayout();
}
void CommandBuffer::PushConstants::bake(VkCommandBuffer commandBuffer) {
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdPushConstants(commandBuffer, layout, stages, offset, data.size(), data.data());
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::PushConstants::toString(bool includeArguments) {
  return "vkCmdPushConstants";
}

void CommandBuffer::record(const Command& queuedCommand) {
  // Add pipeline barrier commands
  if (flags & AddPipelineBarriers) {
    // Create the PipelineBarrier command
    auto pipelineBarrier = std::make_unique<PipelineBarrier>(0, 0, 0, std::span<PipelineBarrier::MemoryBarrier>{}, std::span<PipelineBarrier::BufferMemoryBarrier>{}, std::span<PipelineBarrier::ImageMemoryBarrier>{});

    // Add necessary synchronization items to it
    for (const Command::ResourceAccess& access: queuedCommand.accesses) {
      const Resource::Type type = access.resource->type;

      // Get the last resource access or initialize an undefined one if no previous access to the resource was made
      const auto it                = state.resourceStates.find(access.resource);
      ResourceState& resourceState = (it == state.resourceStates.end() ? state.resourceStates.emplace(access.resource, ResourceState{}).first : it)->second;
      // If we have never referenced this resource before and now we are trying to sample it, assume it is a texture that has been properly initialized.
      if (resourceState.layout == VK_IMAGE_LAYOUT_UNDEFINED && std::ranges::contains(access.allowedLayouts, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
        resourceState = {
          .access = nullptr,
          .stage = nullptr,
          .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
      }

      const bool srcWrite = resourceState.access == nullptr ? true : vkuAccessIsWrite(*resourceState.access);
      const bool dstWrite = vkuAccessIsWrite(access.mask);

      switch (type) {
        case Resource::Type::Image: {
          // Obtain a barrier
          const auto* const image = reinterpret_cast<const Image* const>(access.resource);
          PipelineBarrier::ImageMemoryBarrier& barrier = pipelineBarrier->imageMemoryBarriers.emplace_back(PipelineBarrier::ImageMemoryBarrier{
              .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
              .pNext               = nullptr,
              .srcAccessMask       = 0,
              .dstAccessMask       = access.mask,
              .oldLayout           = resourceState.layout,
              .newLayout           = access.allowedLayouts[0],  // We always choose the layout listed first.
                                                                // todo: Prefer a layout that reduces the number of memory barriers / transitions needed most if one exists.
                                                                //   This would probably involve choosing the first layout, then when the next access happens, come back here and choose a layout that would not require a transition if one exists. This command could then be removed if not needed.
                                                                //   That could happen again for the next access after that one too if we had successfully chosen a layout that does not require a transition.
              .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .image               = image,
              .subresourceRange    = image->getWholeRange()
          });

          // Check if this resource requires synchronization for this operation
          if (!srcWrite && !dstWrite &&  // READ -> READ
              !std::ranges::contains(access.allowedLayouts, resourceState.layout)) {  // No layout transition is required
            // Accumulate this operation as it does not require explicit synchronization separate from other reads
            *resourceState.access |= access.mask;
            *resourceState.stage  |= access.stage;
            pipelineBarrier->imageMemoryBarriers.pop_back();
            continue;  // Do not generate READ -> READ synchronization barriers
          }
          // Synchronization required
          pipelineBarrier->dstStageMask |= access.stage;

          barrier.srcAccessMask = resourceState.access != nullptr ? *resourceState.access : 0;
          pipelineBarrier->srcStageMask |= resourceState.stage != nullptr ? *resourceState.stage : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

          // Update the resource state
          resourceState.access = &barrier.dstAccessMask;
          resourceState.stage  = &pipelineBarrier->dstStageMask;
          resourceState.layout = barrier.newLayout;
          break;
        }
        case Resource::Type::Buffer: {
          const auto* const buffer = reinterpret_cast<const Buffer* const>(access.resource);
          PipelineBarrier::BufferMemoryBarrier& barrier = pipelineBarrier->bufferMemoryBarriers.emplace_back(PipelineBarrier::BufferMemoryBarrier{
              .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
              .pNext               = nullptr,
              .srcAccessMask       = 0,
              .dstAccessMask       = access.mask,
              .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
              .buffer              = buffer,
              .offset              = 0,
              .size                = VK_WHOLE_SIZE
          });

          // Check if this resource requires synchronization for this operation
          if (!srcWrite && !dstWrite) {  // READ -> READ
            // Accumulate this operation as it does not require explicit synchronization separate from other reads
            *resourceState.access |= access.mask;
            *resourceState.stage  |= access.stage;
            pipelineBarrier->bufferMemoryBarriers.pop_back();
            continue;  // Do not generate READ -> READ synchronization barriers
          }
          // Synchronization required
          pipelineBarrier->dstStageMask |= access.stage;

          barrier.srcAccessMask = resourceState.access != nullptr ? *resourceState.access : 0;
          pipelineBarrier->srcStageMask |= resourceState.stage != nullptr ? *resourceState.stage : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

          // Update the resource state
          resourceState.access = &barrier.dstAccessMask;
          resourceState.stage  = &pipelineBarrier->dstStageMask;
          break;
        }
      }
    }
    // Do not record the barrier if it is empty
    if (pipelineBarrier->memoryBarriers.empty() && pipelineBarrier->bufferMemoryBarriers.empty() && pipelineBarrier->imageMemoryBarriers.empty()) return;
    commands.emplace_back(std::move(pipelineBarrier))->preprocess(state, flags);
  }

  /**@todo: Merge Copy*To* commands.*/
}

void CommandBuffer::addCleanupResource(Resource* resource) {
  rawResources.insert(resource);
}

void CommandBuffer::addCleanupResource(const std::shared_ptr<Resource>& resource) {
  sharedResources.insert(resource);
}

void CommandBuffer::addCleanupResource(std::unique_ptr<Resource>& resource) {
  uniqueResources.insert(std::move(resource));
}

std::string CommandBuffer::toString() const {
  std::string view;
  uint64_t commandIndex = 0;
  for (const auto& command : commands) {
    view += "#" + std::to_string(commandIndex++) + ": " + command->toString() + "\n";
  }
  return view;
}

void CommandBuffer::bake(VkCommandBuffer commandBuffer) const {
  for (const std::unique_ptr<Command>& command: commands) command->bake(commandBuffer);
}

void CommandBuffer::clear() {
  for (Resource* const& resource: rawResources) delete resource;
  rawResources.clear();
  sharedResources.clear();
  uniqueResources.clear();
  commands.clear();
}

CommandBuffer::~CommandBuffer() {
  for (Resource* const& resource: rawResources) delete resource;
}
