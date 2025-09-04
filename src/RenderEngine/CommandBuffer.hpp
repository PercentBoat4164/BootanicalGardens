#pragma once

#include "RenderPass/RenderPass.hpp"
#include "Resources/Buffer.hpp"

#include <vulkan/vulkan_core.h>

#include <cpptrace/basic.hpp>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class RenderPass;
class Image;
class Buffer;
class Resource;
class Pipeline;
class Mesh;

class CommandBuffer {
public:
  enum PreprocessingFlags {
    AddPipelineBarriers = 0x1,
    RemovePipelineBarriers = 0x2,
    ModifyPipelineBarriers = 0x4,
    MergePipelineBarriers = 0x8,
    PipelineBarriers = AddPipelineBarriers | RemovePipelineBarriers | ModifyPipelineBarriers | MergePipelineBarriers,
    MergePipelineBinds = 0x10,
    CommandBufferStateTransitions = 0x10,
    Everything = PipelineBarriers | CommandBufferStateTransitions
  };

  struct ResourceState {
    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkAccessFlags access{VK_ACCESS_FLAG_BITS_MAX_ENUM};
  };

  struct State {
    std::unordered_map<void*, ResourceState> resourceStates;
    std::weak_ptr<RenderPass> renderPass;
    std::weak_ptr<Pipeline> pipeline;
    std::weak_ptr<Buffer> indexBuffer;
    std::vector<std::weak_ptr<Buffer>> vertexBuffers;
  };

  struct Command {
    struct ResourceAccess {
      enum TypeBits : uint8_t {
        Read  = 1 << 0,
        Write = 1 << 1
      };
      using Type = uint8_t;
      Type type{Read};
      Resource* resource{nullptr};
      VkPipelineStageFlags stage{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
      VkAccessFlags mask{VK_ACCESS_NONE};
      std::vector<VkImageLayout> allowedLayouts{};
    };
    std::vector<ResourceAccess> accesses;
    enum Type : uint8_t {
      Synchronization,
      StateChange,
      Copy,
      Draw
    } type;
#if BOOTANICAL_GARDENS_ENABLE_COMMAND_BUFFER_TRACING
    cpptrace::raw_trace trace;
#endif

    explicit Command(std::vector<ResourceAccess> accesses, Type type);
    virtual ~Command() = default;

  protected:
    virtual std::string toString(bool includeArguments=false)              = 0;
    virtual void        preprocess(State& state, PreprocessingFlags flags) = 0;
    virtual void        bake(VkCommandBuffer commandBuffer)                = 0;

    friend CommandBuffer;
  };

private:
  std::list<std::shared_ptr<Command>> commands;

public:
  using iterator = decltype(commands)::iterator;
  using reverse_iterator = decltype(commands)::reverse_iterator;
  using const_iterator = decltype(commands)::const_iterator;
  using const_reverse_iterator = decltype(commands)::const_reverse_iterator;

  iterator begin() { return commands.begin(); }
  iterator end() { return commands.end(); }
  reverse_iterator rbegin() { return commands.rbegin(); }
  reverse_iterator rend() { return commands.rend(); }
  [[nodiscard]] const_iterator cbegin() const { return commands.cbegin(); }
  [[nodiscard]] const_iterator cend() const { return commands.cend(); }
  [[nodiscard]] const_reverse_iterator crbegin() const { return commands.crbegin(); }
  [[nodiscard]] const_reverse_iterator crend() const { return commands.crend(); }
  [[nodiscard]] bool empty() const { return commands.empty(); }

  struct BeginRenderPass final : Command {
    explicit BeginRenderPass(std::shared_ptr<RenderPass> renderPass, VkRect2D renderArea={}, std::vector<VkClearValue> clearValues={});
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<RenderPass> renderPass;
    VkRect2D renderArea;
    std::vector<VkClearValue> clearValues;
    VkRenderPassBeginInfo renderPassBeginInfo;
  };

  struct BindDescriptorSets final : Command {
    explicit BindDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets, uint32_t firstSet=0);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::vector<VkDescriptorSet> descriptorSets;
    uint32_t firstSet;
    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  };

  struct BindIndexBuffers final : Command {
    explicit BindIndexBuffers(std::shared_ptr<Buffer> indexBuffer);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Buffer> buffer;
  };

  struct BindPipeline final : Command {
    explicit BindPipeline(std::shared_ptr<Pipeline> pipeline);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Pipeline> pipeline;
    VkExtent2D extent;
  };

  struct BindVertexBuffers final : Command {
    explicit BindVertexBuffers(const std::vector<std::tuple<std::shared_ptr<Buffer>, const VkDeviceSize>>& vertexBuffers);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::vector<std::shared_ptr<Buffer>> buffers;
    std::vector<VkBuffer> rawBuffers;
    std::vector<VkDeviceSize> offsets;
  };

  struct BlitImageToImage final : Command {
    BlitImageToImage(std::shared_ptr<Image> src, std::shared_ptr<Image> dst, std::vector<VkImageBlit> regions={}, VkFilter filter=VK_FILTER_NEAREST);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Image> src;
    std::shared_ptr<Image> dst;

    VkImage srcImage{VK_NULL_HANDLE};
    VkImage dstImage{VK_NULL_HANDLE};
    VkImageLayout srcImageLayout{};
    VkImageLayout dstImageLayout{};
    std::vector<VkImageBlit> regions{};
    VkFilter filter{};
  };

  struct ClearColorImage final : Command {
    explicit ClearColorImage(std::shared_ptr<Image> img, VkClearColorValue value={}, std::vector<VkImageSubresourceRange> subresourceRanges={});
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Image> img;
    VkClearColorValue value;
    std::vector<VkImageSubresourceRange> subresourceRanges;
    VkImage image{VK_NULL_HANDLE};
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct ClearDepthStencilImage final : Command {
    explicit ClearDepthStencilImage(std::shared_ptr<Image> img, VkClearDepthStencilValue value={1.0, 0}, std::vector<VkImageSubresourceRange> subresourceRanges={});
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Image> img;
    VkClearDepthStencilValue value;
    std::vector<VkImageSubresourceRange> subresourceRanges;
    VkImage image{VK_NULL_HANDLE};
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct CopyBufferToBuffer final : Command {
    CopyBufferToBuffer(std::shared_ptr<Buffer> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferCopy> regions={});
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Buffer> src;
    std::shared_ptr<Buffer> dst;
    std::vector<VkBufferCopy> regions;
    VkBuffer srcBuffer{VK_NULL_HANDLE};
    VkBuffer dstBuffer{VK_NULL_HANDLE};
  };

  struct CopyBufferToImage final : Command {
    CopyBufferToImage(std::shared_ptr<Buffer> src, std::shared_ptr<Image> dst, std::vector<VkBufferImageCopy> regions={});
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Buffer> src;
    std::shared_ptr<Image> dst;
    std::vector<VkBufferImageCopy> regions;
    VkBuffer buffer{VK_NULL_HANDLE};
    VkImage image{VK_NULL_HANDLE};
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct CopyImageToBuffer final : Command {
    CopyImageToBuffer(std::shared_ptr<Image> src, std::shared_ptr<Buffer> dst, std::vector<VkBufferImageCopy> regions={});
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Image> src;
    std::shared_ptr<Buffer> dst;
    std::vector<VkBufferImageCopy> regions;
    VkImage image{VK_NULL_HANDLE};
    VkBuffer buffer{VK_NULL_HANDLE};
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct CopyImageToImage final : Command {
    CopyImageToImage(std::shared_ptr<Image> src, std::shared_ptr<Image> dst, std::vector<VkImageCopy> regions={});
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Image> src;
    std::shared_ptr<Image> dst;
    std::vector<VkImageCopy> regions;
    VkImage srcImage{VK_NULL_HANDLE};
    VkImage dstImage{VK_NULL_HANDLE};
    VkImageLayout srcLayout{VK_IMAGE_LAYOUT_MAX_ENUM};
    VkImageLayout dstLayout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct Draw final : Command {
    explicit Draw(uint32_t vertexCount=0);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    uint32_t vertexCount{0};
  };

  struct DrawIndexed final : Command {
    explicit DrawIndexed();
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    uint32_t vertexCount{0};
    uint32_t indexCount{0};
  };

  struct DrawIndexedIndirect final : Command {
    explicit DrawIndexedIndirect(const std::shared_ptr<Buffer>& buffer, uint32_t count, VkDeviceSize offset=0, uint32_t stride=sizeof(VkDrawIndexedIndirectCommand));
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::shared_ptr<Buffer> buffer;
    VkBuffer buf{VK_NULL_HANDLE};
    VkDeviceSize offset{0};
    uint32_t count{0};
    uint32_t stride{0};
  };

  struct EndRenderPass final : Command {
    explicit EndRenderPass();
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;
  };

  struct PipelineBarrier final : Command {
    PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, std::vector<VkMemoryBarrier> memoryBarriers, std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers, std::vector<VkImageMemoryBarrier> imageMemoryBarriers);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkDependencyFlags dependencyFlags;
    std::vector<VkMemoryBarrier> memoryBarriers;
    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
    std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
  };

  template<typename T, typename... Args> requires std::constructible_from<T, Args...> && std::derived_from<T, Command> && (!std::is_same_v<T, Command>) const_iterator record(const const_iterator& iterator, Args&&... args) { return commands.insert(iterator, std::make_unique<T>(std::forward<Args&&>(args)...)); }
  template<typename T, typename... Args> requires std::constructible_from<T, Args...> && std::derived_from<T, Command> && (!std::is_same_v<T, Command>) const_iterator record(Args&&... args) { return record<T>(commands.cend(), std::forward<Args&&>(args)...); }
  const_iterator record(const const_iterator& iterator, const CommandBuffer& commandBuffer);
  const_iterator record(const CommandBuffer& commandBuffer);
  /**
   * This command will preprocess this CommandBuffer assuming a set of input states. This process involves not only modifying the recorded commands to be correct with each other, but can optimize them as well.
   * @param state The states of all resources as they will be at the time this command buffer is submitted to the GPU
   * @param flags The optimizations to perform
   * @param apply Whether changes should be applied to this command buffer or not
   * @return The states of all resources as they will be at the time this command buffer finishes executing on the GPU (Assuming it is the only executing command buffer)
   */
  State preprocess(State state={}, PreprocessingFlags flags=Everything, bool apply=true);

  [[nodiscard]] std::string toString() const;

  /**
   * This command will record all of this CommandBuffer's stored commands into a VkCommandBuffer.
   * @param commandBuffer The VkCommandBuffer to record commands into
   */
  void bake(VkCommandBuffer commandBuffer) const;
  void clear();

  void getDefaultState(State&state);
};