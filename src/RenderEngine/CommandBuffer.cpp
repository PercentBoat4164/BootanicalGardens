#include "CommandBuffer.hpp"

#include "Resource.hpp"
#include "Buffer.hpp"
#include "Image.hpp"

#include <volk.h>

#include <utility>
#include <algorithm>

CommandBuffer::Command::Command(std::vector<ResourceAccess> accesses) : accesses(std::move(accesses)) {}

CommandBuffer::CopyBufferToBuffer::CopyBufferToBuffer(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferCopy> regions) :
    Command({ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT},
             ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
CommandBuffer::CopyBufferToBuffer* CommandBuffer::CopyBufferToBuffer::polymorphicCopy() { return new CopyBufferToBuffer{*this}; }
void CommandBuffer::CopyBufferToBuffer::bake(VkCommandBuffer commandBuffer) {
  vkCmdCopyBuffer(commandBuffer, src->buffer(), dst->buffer(), regions.size(), regions.data());
}

CommandBuffer::CopyBufferToImage::CopyBufferToImage(std::shared_ptr<Buffer> src, std::shared_ptr<Image> dst, std::vector<VkBufferImageCopy> regions) :
    Command({ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT},
             ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
CommandBuffer::CopyBufferToImage* CommandBuffer::CopyBufferToImage::polymorphicCopy() { return new CopyBufferToImage{*this}; }
void CommandBuffer::CopyBufferToImage::bake(VkCommandBuffer commandBuffer) {
  vkCmdCopyBufferToImage(commandBuffer, src->buffer(), dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
}

CommandBuffer::CopyImageToBuffer::CopyImageToBuffer(std::shared_ptr<Image> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferImageCopy> regions) :
    Command({ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
             ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
CommandBuffer::CopyImageToBuffer* CommandBuffer::CopyImageToBuffer::polymorphicCopy() { return new CopyImageToBuffer{*this}; }
void CommandBuffer::CopyImageToBuffer::bake(VkCommandBuffer commandBuffer) {
  vkCmdCopyImageToBuffer(commandBuffer, src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->buffer(), regions.size(), regions.data());
}

CommandBuffer::CopyImageToImage::CopyImageToImage(std::shared_ptr<Image> src, std::shared_ptr<Image> dst, std::vector<VkImageCopy> regions) :
    Command(src == dst ? std::vector{ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
                                     ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}
                       : std::vector{ResourceAccess{ResourceAccess::Read, src.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
                                     ResourceAccess{ResourceAccess::Write, dst.get(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}),
    src(std::move(src)),
    dst(std::move(dst)),
    regions(std::move(regions)) {}
CommandBuffer::CopyImageToImage* CommandBuffer::CopyImageToImage::polymorphicCopy() { return new CopyImageToImage{*this}; }
void CommandBuffer::CopyImageToImage::bake(VkCommandBuffer commandBuffer) {
  vkCmdCopyImage(commandBuffer, src->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
}

CommandBuffer::PipelineBarrier::PipelineBarrier(const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask, const VkDependencyFlags dependencyFlags, std::vector<VkMemoryBarrier> memoryBarriers, std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers, std::vector<VkImageMemoryBarrier> imageMemoryBarriers) :
    Command({}),
    srcStageMask(srcStageMask),
    dstStageMask(dstStageMask),
    dependencyFlags(dependencyFlags),
    memoryBarriers(std::move(memoryBarriers)),
    bufferMemoryBarriers(std::move(bufferMemoryBarriers)),
    imageMemoryBarriers(std::move(imageMemoryBarriers)) {}
CommandBuffer::PipelineBarrier* CommandBuffer::PipelineBarrier::polymorphicCopy() { return new PipelineBarrier{*this}; }
void CommandBuffer::PipelineBarrier::bake(VkCommandBuffer commandBuffer) {
  vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarriers.size(), memoryBarriers.data(), bufferMemoryBarriers.size(), bufferMemoryBarriers.data(), imageMemoryBarriers.size(), imageMemoryBarriers.data());
}

void CommandBuffer::record(const CommandBuffer& commandBuffer) {
  for (const std::unique_ptr<Command>& command: commandBuffer.commands)
    commands.emplace_front(command->polymorphicCopy());
}

void CommandBuffer::optimize(const std::unordered_map<Resource*, ResourceState>& resourceStates) {
  std::unordered_map<Resource*, ResourceState> states = resourceStates;
  /**@todo Make this able to change Blits to Copies and Copies to Blits based on what is possible and speed.*/
  /**@todo Think about VK_DEPENDENCY_BY_REGION_BIT. This should only really affect tiled GPUs.*/
  std::unordered_map<Resource*, Command::ResourceAccess> previousAccesses;
  // Add PipelineBarrier commands
  for (auto commandIterator{commands.begin()}; commandIterator != commands.end(); ++commandIterator) {
    for (const Command::ResourceAccess& access : (*commandIterator)->accesses) {
      ResourceState& state = states[access.resource];
      Command::ResourceAccess& previousAccess = previousAccesses[access.resource];
      std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
      if (access.resource->type == Resource::Image && state.layout != VK_IMAGE_LAYOUT_MAX_ENUM && !access.allowedLayouts.empty() && !std::ranges::contains(access.allowedLayouts, state.layout)) {
        Image& image = *dynamic_cast<Image*>(access.resource);
        imageMemoryBarriers.push_back({
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .pNext = nullptr,
          .srcAccessMask = access.mask,
          .dstAccessMask = access.mask,
          .oldLayout = state.layout,
          .newLayout = access.allowedLayouts[0],  // We always prefer the layout listed first.
          .srcQueueFamilyIndex = access.resource->device.globalQueueFamilyIndex,  /**@todo This must be changed when I decide to add extra queues.*/
          .dstQueueFamilyIndex = access.resource->device.globalQueueFamilyIndex,
          .image = image.image(),
          .subresourceRange = {
            .aspectMask = image.aspect(),
            .baseMipLevel = 0,
            .levelCount = image.mipLevels(),
            .baseArrayLayer = 0,
            .layerCount = image.layerCount(),
          }
        });
        state.layout = access.allowedLayouts[0];
      }
      commands.insert_after(commandIterator++, std::make_unique<PipelineBarrier>(previousAccess.stage, access.stage, 0, std::vector<VkMemoryBarrier>{}, std::vector<VkBufferMemoryBarrier>{}, imageMemoryBarriers));
      previousAccess = access;
    }
  }
  //@todo Merge PipelineBarrier commands
}

void CommandBuffer::bake(VkCommandBuffer commandBuffer) const {
  for (const std::unique_ptr<Command>& command: commands)
    command->bake(commandBuffer);
}

void CommandBuffer::clear() {
  commands.clear();
}
