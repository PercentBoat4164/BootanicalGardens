#pragma once

#include <vulkan/vulkan_core.h>

#include <forward_list>
#include <memory>
#include <unordered_map>
#include <vector>

class RenderPass;
class Image;
class Buffer;
class Resource;

class CommandBuffer {
  struct Command {
    struct ResourceAccess {
      enum Type : bool {Read, Write} type{Read};
      Resource* resource{nullptr};
      VkPipelineStageFlags stage{VK_PIPELINE_STAGE_NONE};
      VkAccessFlags mask{VK_ACCESS_NONE};
      std::vector<VkImageLayout> allowedLayouts{};
    };

    explicit Command(std::vector<ResourceAccess> accesses);
    virtual ~Command() = default;
    virtual Command* polymorphicCopy() = 0;

    virtual void bake(VkCommandBuffer commandBuffer) = 0;

    std::vector<ResourceAccess> accesses;
  };

  struct ResourceState {
    VkImageLayout layout;
  };

  std::forward_list<std::unique_ptr<Command>> commands;

public:
  struct CopyBufferToBuffer final : Command {
    CopyBufferToBuffer(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferCopy> regions);
    CopyBufferToBuffer* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer) override;

    std::shared_ptr<Buffer> src;
    std::shared_ptr<Buffer> dst;
    std::vector<VkBufferCopy> regions;
  };

  struct CopyBufferToImage final : Command {
    CopyBufferToImage(std::shared_ptr<Buffer> src, std::shared_ptr<Image> dst, std::vector<VkBufferImageCopy> regions);
    CopyBufferToImage* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer) override;

    std::shared_ptr<Buffer> src;
    std::shared_ptr<Image> dst;
    std::vector<VkBufferImageCopy> regions;
  };

  struct CopyImageToBuffer final : Command {
    CopyImageToBuffer(std::shared_ptr<Image> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferImageCopy> regions);
    CopyImageToBuffer* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer) override;

    std::shared_ptr<Image> src;
    std::shared_ptr<Buffer> dst;
    std::vector<VkBufferImageCopy> regions;
  };

  struct CopyImageToImage final : Command {
    CopyImageToImage(std::shared_ptr<Image> src, std::shared_ptr<Image> dst, std::vector<VkImageCopy> regions);
    CopyImageToImage* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer) override;

    std::shared_ptr<Image> src;
    std::shared_ptr<Image> dst;
    std::vector<VkImageCopy> regions;
  };

  struct PipelineBarrier final : Command {
    PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, std::vector<VkMemoryBarrier> memoryBarriers, std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers, std::vector<VkImageMemoryBarrier> imageMemoryBarriers);
    PipelineBarrier* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer) override;

    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkDependencyFlags dependencyFlags;
    std::vector<VkMemoryBarrier> memoryBarriers;
    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
    std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
  };

  /**@todo Allow inserting commands and other command buffers into command buffers at arbitrary points.*/
  template<typename T, typename... Args> requires std::constructible_from<T, Args...> && std::derived_from<T, Command> && std::derived_from<std::is_same<T, Command>, std::false_type> void record(Args&&... args) { commands.push_front(std::make_unique<T>(std::forward<Args&&>(args)...)); }
  void record(const CommandBuffer& commandBuffer);
  void optimize(const std::unordered_map<Resource*, ResourceState>& resourceStates);
  void bake(VkCommandBuffer commandBuffer) const;
  void clear();
};
