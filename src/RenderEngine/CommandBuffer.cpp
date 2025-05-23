#include "src/RenderEngine/CommandBuffer.hpp"

#include "Resources/Buffer.hpp"
#include "Resources/Image.hpp"
#include "Resources/Resource.hpp"
#include "src/RenderEngine/Framebuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/Renderable/Vertex.hpp"

#include <volk/volk.h>
#include <vulkan/utility/vk_format_utils.h>

#include <algorithm>
#include <ranges>
#include <utility>
#include <iostream>

CommandBuffer::Command::Command(std::vector<ResourceAccess> accesses) : accesses(std::move(accesses))
#ifndef NDEBUG
, stacktrace(cpptrace::generate_trace())
#endif
{}

CommandBuffer::BeginRenderPass::BeginRenderPass(std::shared_ptr<RenderPass> renderPass, const VkRect2D renderArea, std::vector<VkClearValue> clearValues) :
    Command([&]->std::vector<ResourceAccess>{
      std::vector<ResourceAccess> accesses;
      auto attachments = renderPass->declareAttachments();
      uint32_t index{};
      for (std::shared_ptr<Image> image : renderPass->getFramebuffer()->getImages()) {
        auto& attachment = attachments[index++].second;
        accesses.emplace_back(ResourceAccess::Write, image.get(), attachment.stage, attachment.access, std::vector{attachment.layout});
        accesses.emplace_back(ResourceAccess::Write | ResourceAccess::Read, image.get(), attachment.stage, attachment.access, std::vector{attachment.layout});
      }
      return accesses;
    }()),
    renderPass(std::move(renderPass)),
    renderArea(renderArea.offset.x == 0 && renderArea.offset.y == 0 && renderArea.extent.width == 0 && renderArea.extent.height == 0 ? this->renderPass->getFramebuffer()->getRect() : renderArea),
    clearValues(std::move(clearValues)),
    renderPassBeginInfo() {}
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
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BeginRenderPass::toString(const bool includeArguments) {
  if (!includeArguments) return "vkCmdBeginRenderPass";
  std::string string = "vkCmdBeginRenderPass\n"
  "\tRender Pass: " + std::to_string(reinterpret_cast<uint64_t>(renderPass.get())) + "\n"
  "\tRender Area:\n"
  "\t\textent: " + std::to_string(renderArea.extent.width) + "x" + std::to_string(renderArea.extent.height) + "\n"
  "\t\toffset: (" + std::to_string(renderArea.offset.x) + ", " + std::to_string(renderArea.offset.y) + ")";
  if (!clearValues.empty()) {
    string += "\n\tClear Values (" + std::to_string(clearValues.size()) + "):";
    uint64_t index{};
    for (const VkClearValue& value: clearValues) {
      string += "\n\t\t#" + std::to_string(index++) + ":\n"
      "\t\t\tcolor: [" + std::to_string(value.color.float32[0]) + ", " + std::to_string(value.color.float32[1]) + ", " + std::to_string(value.color.float32[2]) + ", " + std::to_string(value.color.float32[3]) + "]\n"
      "\t\t\tdepth: [" + std::to_string(value.depthStencil.depth) + ", " + std::to_string(value.depthStencil.stencil);
    }
  }
  return string;
}

CommandBuffer::BindDescriptorSets::BindDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets) :
    Command({}),
    descriptorSets(descriptorSets) {}
void CommandBuffer::BindDescriptorSets::preprocess(State& state, PreprocessingFlags flags) {
  /**@todo: This should not rely on the currently bound pipeline because the descriptor sets may be bound before the pipeline is. Add an algorithm that sets this layout correctly no matter the bind order.
   *   - Look for the next bind descriptor set call that binds the same or more.
   *   - Check the pipeline that is bound during each draw call issued before that descriptor set binding.
   *   - Ensure they all of those pipelines have a compatible layout.
   *       - If they do not this command buffer should be un-bake-able as this would result in illegal state in Vulkan.
   *   - Choose the pipeline layout of any of the checked pipelines (they are all compatible, so which one is selected does not matter).*/
  pipelineLayout = state.pipeline.lock()->getLayout();
}
void CommandBuffer::BindDescriptorSets::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindDescriptorSets::toString(bool includeArguments) {
  return "vkCmdBindDescriptorSets";
}

CommandBuffer::BindIndexBuffers::BindIndexBuffers(std::shared_ptr<Buffer> indexBuffer) :
    Command({}),
    buffer(std::move(indexBuffer)) {}
void CommandBuffer::BindIndexBuffers::preprocess(State& state, PreprocessingFlags flags) {
  state.indexBuffer = buffer;
}
void CommandBuffer::BindIndexBuffers::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBindIndexBuffer(commandBuffer, buffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32); /**@todo: Expose these options.*/
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindIndexBuffers::toString(bool includeArguments) {
  return "vkCmdBindIndexBuffer";
}

CommandBuffer::BindPipeline::BindPipeline(std::shared_ptr<Pipeline> pipeline) :
    Command({}),
    pipeline(std::move(pipeline)),
    extent{} {}
void CommandBuffer::BindPipeline::preprocess(State& state, PreprocessingFlags flags) {
  state.pipeline = pipeline;
  extent = state.renderPass.lock()->getFramebuffer()->getExtent();
}
void CommandBuffer::BindPipeline::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  /**@todo: Handle dynamic state in other commands.*/
  const VkViewport viewport{0, 0, static_cast<float>(extent.width), static_cast<float>(extent.height), 0, 1};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  const VkRect2D rect{0, 0, extent.width, extent.height};
  vkCmdSetScissor(commandBuffer, 0, 1, &rect);
  vkCmdBindPipeline(commandBuffer, pipeline->bindPoint, pipeline->getPipeline());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindPipeline::toString(bool includeArguments) {
  return "vkCmdBindPipeline";
}

CommandBuffer::BindVertexBuffers::BindVertexBuffers(const std::vector<std::tuple<std::shared_ptr<Buffer>, const VkDeviceSize>>& vertexBuffers) :
    Command({}),
    rawBuffers(vertexBuffers.size()) {
  const auto bufferRange = std::ranges::views::keys(vertexBuffers);
  buffers = {bufferRange.begin(), bufferRange.end()};
  const auto offsetRange = std::ranges::views::values(vertexBuffers);
  offsets = {offsetRange.begin(), offsetRange.end()};
}
void CommandBuffer::BindVertexBuffers::preprocess(State& state, PreprocessingFlags flags) {
  for (uint32_t i{}; i < buffers.size(); ++i) rawBuffers[i] = buffers[i]->getBuffer();
  state.vertexBuffers = {buffers.begin(), buffers.end()};
}
void CommandBuffer::BindVertexBuffers::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBindVertexBuffers(commandBuffer, 0, rawBuffers.size(), rawBuffers.data(), offsets.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BindVertexBuffers::toString(bool includeArguments) {
  return "vkCmdBindVertexBuffers";
}

CommandBuffer::BlitImageToImage::BlitImageToImage(std::shared_ptr<Image> src, std::shared_ptr<Image> dst, std::vector<VkImageBlit> regions, const VkFilter filter) :
    Command({ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
             ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}
    }),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)),
    filter(filter) {}
void CommandBuffer::BlitImageToImage::preprocess(State& state, PreprocessingFlags flags) {
  if (regions.empty()) regions.push_back({
    .srcSubresource = VkImageSubresourceLayers{src->getAspect(), 0, 0, src->getLayerCount()},
    .srcOffsets = {{0, 0, 0}, VkOffset3D{static_cast<int32_t>(src->getExtent().width), static_cast<int32_t>(src->getExtent().height), static_cast<int32_t>(src->getExtent().depth)}},
    .dstSubresource = VkImageSubresourceLayers{dst->getAspect(), 0, 0, dst->getLayerCount()},
    .dstOffsets = {{0, 0, 0}, VkOffset3D{static_cast<int32_t>(dst->getExtent().width), static_cast<int32_t>(dst->getExtent().height), static_cast<int32_t>(dst->getExtent().depth)}},
  });
  srcImage = src->getImage();
  dstImage = dst->getImage();
  srcImageLayout = state.resourceStates.at(srcImage).layout;
  dstImageLayout = state.resourceStates.at(dstImage).layout;
}
void CommandBuffer::BlitImageToImage::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regions.size(), regions.data(), filter);
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::BlitImageToImage::toString(bool includeArguments) {
  return "vkCmdBlitImage";
}

CommandBuffer::ClearColorImage::ClearColorImage(std::shared_ptr<Image> img, const VkClearColorValue value, std::vector<VkImageSubresourceRange> subresourceRanges) :
    Command({ResourceAccess{ResourceAccess::Write, img.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}),
    img(std::move(img)),
    value(value),
    subresourceRanges(std::move(subresourceRanges)) {}
void CommandBuffer::ClearColorImage::preprocess(State& state, PreprocessingFlags flags) {
  if (subresourceRanges.empty()) subresourceRanges.push_back({img->getAspect(), 0, img->getMipLevels(), 0, img->getLayerCount()});
  image = img->getImage();
  layout = state.resourceStates.at(image).layout;
}
void CommandBuffer::ClearColorImage::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdClearColorImage(commandBuffer, image, layout, &value, subresourceRanges.size(), subresourceRanges.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::ClearColorImage::toString(bool includeArguments) {
  return "vkCmdClearColorImage";
}

CommandBuffer::ClearDepthStencilImage::ClearDepthStencilImage(std::shared_ptr<Image> img, const VkClearDepthStencilValue value, std::vector<VkImageSubresourceRange> subresourceRanges) :
    Command({ResourceAccess{ResourceAccess::Write, img.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}),
    img(std::move(img)),
    value(value),
    subresourceRanges(std::move(subresourceRanges)) {}
void CommandBuffer::ClearDepthStencilImage::preprocess(State& state, PreprocessingFlags flags) {
  if (subresourceRanges.empty()) subresourceRanges.push_back({img->getAspect(), 0, img->getMipLevels(), 0, img->getLayerCount()});
  image = img->getImage();
  layout = state.resourceStates.at(image).layout;
}
void CommandBuffer::ClearDepthStencilImage::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdClearDepthStencilImage(commandBuffer, image, layout, &value, subresourceRanges.size(), subresourceRanges.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::ClearDepthStencilImage::toString(bool includeArguments) {
  return "vkCmdClearColorImage";
}

CommandBuffer::CopyBufferToBuffer::CopyBufferToBuffer(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferCopy> regions) :
    Command({ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT},
             ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
void CommandBuffer::CopyBufferToBuffer::preprocess(State& state, PreprocessingFlags flags) {
  if (regions.empty()) regions.push_back({
    .srcOffset = 0,
    .dstOffset = 0,
    .size = std::min(src->getSize(), dst->getSize())
  });
  srcBuffer = src->getBuffer();
  dstBuffer = dst->getBuffer();
}
void CommandBuffer::CopyBufferToBuffer::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regions.size(), regions.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyBufferToBuffer::toString(bool includeArguments) {
  return "vkCmdCopyBuffer";
}

CommandBuffer::CopyBufferToImage::CopyBufferToImage(std::shared_ptr<Buffer> src, std::shared_ptr<Image> dst, std::vector<VkBufferImageCopy> regions) :
    Command({ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT},
             ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
void CommandBuffer::CopyBufferToImage::preprocess(State& state, PreprocessingFlags flags) {
  if (regions.empty()) regions.push_back({
    .bufferOffset = 0,
    .bufferRowLength = vkuFormatTexelBlockExtent(dst->getFormat()).width * dst->getExtent().width,
    .bufferImageHeight = vkuFormatTexelBlockExtent(dst->getFormat()).height * dst->getExtent().height,
    .imageSubresource = VkImageSubresourceLayers{dst->getAspect(), 0, 0, dst->getLayerCount()},
    .imageOffset = VkOffset3D{0, 0, 0},
    .imageExtent = dst->getExtent()
  });
  buffer = src->getBuffer();
  image = dst->getImage();
  layout = state.resourceStates.at(image).layout;
}
void CommandBuffer::CopyBufferToImage::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyBufferToImage(commandBuffer, buffer, image, layout, regions.size(), regions.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyBufferToImage::toString(bool includeArguments) {
  return "vkCmdCopyBufferToImage";
}

CommandBuffer::CopyImageToBuffer::CopyImageToBuffer(std::shared_ptr<Image> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferImageCopy> regions) :
    Command({ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
             ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
void CommandBuffer::CopyImageToBuffer::preprocess(State& state, PreprocessingFlags flags) {
  if (regions.empty()) regions.push_back({
    .bufferOffset = 0,
    .bufferRowLength = vkuFormatTexelBlockExtent(src->getFormat()).width * src->getExtent().width,
    .bufferImageHeight = vkuFormatTexelBlockExtent(src->getFormat()).height * src->getExtent().height,
    .imageSubresource = VkImageSubresourceLayers{src->getAspect(), 0, 0, src->getLayerCount()},
    .imageOffset = VkOffset3D{0, 0, 0},
    .imageExtent = src->getExtent()
  });
  image = src->getImage();
  buffer = dst->getBuffer();
  layout = state.resourceStates.at(image).layout;
}
void CommandBuffer::CopyImageToBuffer::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyImageToBuffer(commandBuffer, image, layout, buffer, regions.size(), regions.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyImageToBuffer::toString(bool includeArguments) {
  return "vkCmdCopyImageToBuffer";
}

CommandBuffer::CopyImageToImage::CopyImageToImage(std::shared_ptr<Image> src, std::shared_ptr<Image> dst, std::vector<VkImageCopy> regions) :
    Command(src == dst ? std::vector{ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
                                     ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}
                       : std::vector{ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
                                     ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
void CommandBuffer::CopyImageToImage::preprocess(State& state, PreprocessingFlags flags) {
  if (regions.empty()) regions.push_back({
    .srcSubresource = VkImageSubresourceLayers{src->getAspect(), 0, 0, src->getLayerCount()},
    .srcOffset      = VkOffset3D{0, 0, 0},
    .dstSubresource = VkImageSubresourceLayers{dst->getAspect(), 0, 0, dst->getLayerCount()},
    .dstOffset      = VkOffset3D{0, 0, 0},
    .extent         = VkExtent3D{std::min(src->getExtent().width, dst->getExtent().width), std::min(src->getExtent().height, dst->getExtent().height), std::min(src->getExtent().depth, dst->getExtent().depth)}
  });
  srcImage = src->getImage();
  dstImage = dst->getImage();
  srcLayout = state.resourceStates.at(srcImage).layout;
  dstLayout = state.resourceStates.at(dstImage).layout;
}
void CommandBuffer::CopyImageToImage::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdCopyImage(commandBuffer, srcImage, srcLayout, dstImage, dstLayout, regions.size(), regions.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::CopyImageToImage::toString(bool includeArguments) {
  return "vkCmdCopyImage";
}

CommandBuffer::Draw::Draw() : Command({}) {}
void CommandBuffer::Draw::preprocess(State& state, PreprocessingFlags flags) {
  if (state.vertexBuffers.empty()) GraphicsInstance::showError("must call BindVertexBuffers before Draw");
  vertexCount = state.vertexBuffers[0].lock()->getSize() / sizeof(Vertex);
}
void CommandBuffer::Draw::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::Draw::toString(bool includeArguments) {
  return "vkCmdDraw";
}

CommandBuffer::DrawIndexed::DrawIndexed() : Command({}) {}
void CommandBuffer::DrawIndexed::preprocess(State& state, PreprocessingFlags flags) {
  if (state.vertexBuffers.empty()) GraphicsInstance::showError("must call BindIndexBuffer before DrawIndexed");
  indexCount = state.indexBuffer.lock()->getSize() / sizeof(uint32_t);
  if (state.vertexBuffers.empty()) GraphicsInstance::showError("must call BindVertexBuffers before DrawIndexed");
  vertexCount = state.vertexBuffers[0].lock()->getSize() / sizeof(Vertex);
}
void CommandBuffer::DrawIndexed::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::DrawIndexed::toString(bool includeArguments) {
  return "vkCmdDrawIndexed";
}

CommandBuffer::EndRenderPass::EndRenderPass() : Command({}) {}
void CommandBuffer::EndRenderPass::preprocess(State& state, PreprocessingFlags flags) {
  state.renderPass.reset();
}
void CommandBuffer::EndRenderPass::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdEndRenderPass(commandBuffer);
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::EndRenderPass::toString(bool includeArguments) {
  return "vkCmdEndRenderPass";
}

CommandBuffer::PipelineBarrier::PipelineBarrier(const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask, const VkDependencyFlags dependencyFlags, std::vector<VkMemoryBarrier> memoryBarriers, std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers, std::vector<VkImageMemoryBarrier> imageMemoryBarriers) :
    Command({}),
    srcStageMask(srcStageMask),
    dstStageMask(dstStageMask),
    dependencyFlags(dependencyFlags),
    memoryBarriers(std::move(memoryBarriers)),
    bufferMemoryBarriers(std::move(bufferMemoryBarriers)),
    imageMemoryBarriers(std::move(imageMemoryBarriers)) {}
void CommandBuffer::PipelineBarrier::preprocess(State& state, PreprocessingFlags flags) {
  if (flags & ModifyPipelineBarriers) {
    for (auto& imageMemoryBarrier: imageMemoryBarriers) {
      ResourceState& resourceState = state.resourceStates.at(imageMemoryBarrier.image);
      if (resourceState.access != VK_ACCESS_FLAG_BITS_MAX_ENUM)
        imageMemoryBarrier.srcAccessMask = resourceState.access;
      resourceState.access = imageMemoryBarrier.dstAccessMask;
      imageMemoryBarrier.oldLayout = resourceState.layout;
      resourceState.layout = imageMemoryBarrier.newLayout;
    }
  }
}
void CommandBuffer::PipelineBarrier::bake(VkCommandBuffer commandBuffer) {
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(this);
#endif
  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarriers.size(), memoryBarriers.data(), bufferMemoryBarriers.size(), bufferMemoryBarriers.data(), imageMemoryBarriers.size(), imageMemoryBarriers.data());
#ifndef NDEBUG
  GraphicsInstance::setDebugDataCommand(nullptr);
#endif
}
std::string CommandBuffer::PipelineBarrier::toString(bool includeArguments) {
  return "vkCmdPipelineBarrier";
}

CommandBuffer::const_iterator CommandBuffer::record(const const_iterator& iterator, const CommandBuffer& commandBuffer) {
  if (commandBuffer.commands.empty()) return commands.cend();
  const_iterator it;
  for (auto& command : commandBuffer.commands) it = commands.insert(iterator, command);
  return ++it;
}

CommandBuffer::const_iterator CommandBuffer::record(const CommandBuffer& commandBuffer) {
  return record(cend(), commandBuffer);
}

CommandBuffer::State CommandBuffer::preprocess(State state, const PreprocessingFlags flags, const bool apply) {
  /**@todo: Make this able to change Blits to Copies where possible for speed.*/
  /**@todo: Make this able to add buffer and global memory barriers.*/
  /**@todo: Think about VK_DEPENDENCY_BY_REGION_BIT. This should only really affect tiled GPUs.*/
  /**@todo: Acknowledge pre-existing PipelineBarrier commands.*/
  if (state.resourceStates.empty())
    state = getDefaultState();
  std::unordered_map<Resource*, Command::ResourceAccess> previousAccesses;
  for (auto commandIterator{commands.begin()}; commandIterator != commands.end(); ++commandIterator) {
    for (const Command::ResourceAccess& access: (*commandIterator)->accesses) {
      ResourceState& resourceState            = state.resourceStates[access.resource->getObject()];
      Command::ResourceAccess& previousAccess = previousAccesses[access.resource];
      if (flags & AddPipelineBarriers) {
        std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
        std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
        if (access.resource->type == Resource::Image && !std::ranges::contains(access.allowedLayouts, resourceState.layout)) {
          Image& image = *dynamic_cast<Image*>(access.resource);
          imageMemoryBarriers.push_back({.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = previousAccess.mask,
            .dstAccessMask       = access.mask,
            .oldLayout           = resourceState.layout == VK_IMAGE_LAYOUT_MAX_ENUM ? VK_IMAGE_LAYOUT_UNDEFINED : resourceState.layout,
            .newLayout           = access.allowedLayouts[0],  // We always prefer the layout listed first.
            .srcQueueFamilyIndex = access.resource->device->globalQueueFamilyIndex,
            .dstQueueFamilyIndex = access.resource->device->globalQueueFamilyIndex,
            .image               = image.getImage(),
            .subresourceRange    = image.getWholeRange()
          });
          resourceState.layout = access.allowedLayouts[0];
        }
        if (access.resource->type == Resource::Buffer) {
          Buffer& buffer = *dynamic_cast<Buffer*>(access.resource);
          bufferMemoryBarriers.push_back({.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, .pNext = nullptr, .srcAccessMask = previousAccess.mask, .dstAccessMask = access.mask, .srcQueueFamilyIndex = access.resource->device->globalQueueFamilyIndex, .dstQueueFamilyIndex = access.resource->device->globalQueueFamilyIndex, .buffer = buffer.getBuffer(), .offset = 0, .size = VK_WHOLE_SIZE});
        }
        if (apply && !imageMemoryBarriers.empty() || !bufferMemoryBarriers.empty()) record<PipelineBarrier>(commandIterator, previousAccess.stage, access.stage, 0, std::vector<VkMemoryBarrier>{}, bufferMemoryBarriers, imageMemoryBarriers);
      }
      previousAccess = access;
    }
    (*commandIterator)->preprocess(state, flags);
  }
  //@todo: Merge PipelineBarrier commands
  return state;
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
  for (std::shared_ptr<Command> command: commands) command->bake(commandBuffer);
}

void CommandBuffer::clear() {
  commands.clear();
}

CommandBuffer::State CommandBuffer::getDefaultState() {
  State state;
  for (auto commandIterator{commands.begin()}; commandIterator != commands.end(); ++commandIterator) {
    for (const Command::ResourceAccess& access : (*commandIterator)->accesses) {
      if (state.resourceStates.contains(access.resource->getObject())) continue;
      switch (access.resource->type) {
        case Resource::Image: state.resourceStates.emplace(access.resource->getObject(), VK_IMAGE_LAYOUT_UNDEFINED); break;
        case Resource::Buffer: state.resourceStates.emplace(access.resource->getObject(), VK_IMAGE_LAYOUT_MAX_ENUM); break;
      }
    }
  }
  return state;
}
