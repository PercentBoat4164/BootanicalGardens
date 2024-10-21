#pragma once

#include <vulkan/vulkan_core.h>

#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

class RenderPass;
class Image;
class Buffer;
class Resource;

class CommandBuffer {
public:
  struct ResourceState {
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };
  using States = std::unordered_map<void*, ResourceState>;

  struct Command {
    struct ResourceAccess {
      enum Type : bool {Read, Write} type{Read};
      Resource* resource{nullptr};
      VkPipelineStageFlags stage{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
      VkAccessFlags mask{VK_ACCESS_NONE};
      std::vector<VkImageLayout> allowedLayouts{};
    };

    explicit Command(std::vector<ResourceAccess> accesses);
    virtual ~Command() = default;
    virtual Command* polymorphicCopy() = 0;

    virtual void bake(VkCommandBuffer commandBuffer, States& states) = 0;

    std::vector<ResourceAccess> accesses;
  };

private:
  std::list<std::unique_ptr<Command>> commands;

public:
  struct CopyBufferToBuffer final : Command {
    CopyBufferToBuffer(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferCopy> regions);
    CopyBufferToBuffer* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer, States& states) override;

    std::shared_ptr<Buffer> src;
    std::shared_ptr<Buffer> dst;
    std::vector<VkBufferCopy> regions;
  };

  struct CopyBufferToImage final : Command {
    CopyBufferToImage(std::shared_ptr<Buffer> src, std::shared_ptr<Image> dst, std::vector<VkBufferImageCopy> regions);
    CopyBufferToImage* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer, States& states) override;

    std::shared_ptr<Buffer> src;
    std::shared_ptr<Image> dst;
    std::vector<VkBufferImageCopy> regions;
  };

  struct CopyImageToBuffer final : Command {
    CopyImageToBuffer(std::shared_ptr<Image> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferImageCopy> regions);
    CopyImageToBuffer* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer, States& states) override;

    std::shared_ptr<Image> src;
    std::shared_ptr<Buffer> dst;
    std::vector<VkBufferImageCopy> regions;
  };

  struct CopyImageToImage final : Command {
    CopyImageToImage(std::shared_ptr<Image> src, std::shared_ptr<Image> dst, std::vector<VkImageCopy> regions);
    CopyImageToImage* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer, States& states) override;

    std::shared_ptr<Image> src;
    std::shared_ptr<Image> dst;
    std::vector<VkImageCopy> regions;
  };

  struct PipelineBarrier final : Command {
    PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, std::vector<VkMemoryBarrier> memoryBarriers, std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers, std::vector<VkImageMemoryBarrier> imageMemoryBarriers);
    PipelineBarrier* polymorphicCopy() override;

    void bake(VkCommandBuffer commandBuffer, States& states) override;

    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkDependencyFlags dependencyFlags;
    std::vector<VkMemoryBarrier> memoryBarriers;
    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
    std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
  };

  template<typename T, typename... Args> requires std::constructible_from<T, Args...> && std::derived_from<T, Command> && (!std::is_same_v<T, Command>) decltype(commands)::const_iterator record(const decltype(commands)::const_iterator iterator, Args&&... args) { return commands.insert(iterator, std::make_unique<T>(std::forward<Args&&>(args)...)); }
  template<typename T, typename... Args> requires std::constructible_from<T, Args...> && std::derived_from<T, Command> && (!std::is_same_v<T, Command>) decltype(commands)::const_iterator record(Args&&... args) { return record<T>(commands.cend(), std::forward<Args&&>(args)...); }
  decltype(commands)::const_iterator record(decltype(commands)::const_iterator iterator, const CommandBuffer& commandBuffer);
  decltype(commands)::const_iterator record(const CommandBuffer& commandBuffer);
  States optimize(States states={});
  States bake(VkCommandBuffer commandBuffer, States states={}) const;
  void clear();
};