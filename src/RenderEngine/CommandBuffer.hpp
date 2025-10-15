#pragma once

#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/Resources/Image.hpp"

#include <vulkan/vulkan_core.h>

#include <cpptrace/basic.hpp>

#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include <ranges>

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
    StateTransitions = MergePipelineBinds,
    Everything = PipelineBarriers | StateTransitions
  };

  struct ResourceState {
    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkAccessFlags access{VK_ACCESS_FLAG_BITS_MAX_ENUM};
  };

  struct State {
    std::unordered_map<const Resource*, ResourceState> resourceStates;
    const RenderPass* renderPass;
    const Pipeline* pipeline;
    const Buffer* indexBuffer;
    std::vector<const Buffer*> vertexBuffers;
  };

  struct Command {
    struct ResourceAccess {
      enum TypeBits : uint8_t {
        Read  = 1 << 0,
        Write = 1 << 1
      };
      using Type = uint8_t;
      Type type{Read};
      const Resource* resource{nullptr};
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
  std::list<std::unique_ptr<Command>> commands;
  plf::colony<Resource*> resources;

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
    template<std::ranges::range T = std::span<VkClearValue>>
    /**@todo: Make renderPass const when declareAccesses has been made constable*/
    explicit BeginRenderPass(RenderPass* renderPass, T&& clearValues=T{}, const VkRect2D renderArea={}) :
        Command([&]->std::vector<ResourceAccess>{
          std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> attachments = renderPass->declareAccesses();
          std::vector<ResourceAccess> accesses;
          accesses.reserve(attachments.size());
          for (const auto& [id, access]: attachments) {
            accesses.emplace_back(ResourceAccess::Write | ResourceAccess::Read, renderPass->getGraph().getImage(id).image, access.stage, access.access, std::vector{access.layout});
          }
          return accesses;
        }(), StateChange),
        renderPass(renderPass),
        renderArea(renderArea.offset.x == 0 && renderArea.offset.y == 0 && renderArea.extent.width == 0 && renderArea.extent.height == 0 ? this->renderPass->getFramebuffer()->getRect() : renderArea),
        clearValues(std::ranges::to<std::vector>(clearValues)),
        renderPassBeginInfo() {}
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    RenderPass* renderPass;
    VkRect2D renderArea;
    std::vector<VkClearValue> clearValues;
    VkRenderPassBeginInfo renderPassBeginInfo;
  };

  struct BindDescriptorSets final : Command {
    explicit BindDescriptorSets(std::ranges::range auto&& descriptorSets, const uint32_t firstSet=0) :
        Command({}, StateChange),
        descriptorSets{std::ranges::to<std::vector>(descriptorSets)},
        firstSet(firstSet) {}
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::vector<VkDescriptorSet> descriptorSets;
    uint32_t firstSet;
    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  };

  struct BindIndexBuffer final : Command {
    explicit BindIndexBuffer(const Buffer* indexBuffer);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Buffer* buffer;
  };

  struct BindPipeline final : Command {
    explicit BindPipeline(const Pipeline* pipeline);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Pipeline* pipeline;
    VkExtent2D extent;
  };

  struct BindVertexBuffers final : Command {
    explicit BindVertexBuffers(std::ranges::range auto&& vertexBuffers, std::ranges::range auto&& offsets, const uint32_t firstBinding=0) :
        Command({}, StateChange),
        buffers(std::ranges::to<std::vector>(vertexBuffers)),
        offsets(std::ranges::to<std::vector>(offsets)),
        firstBinding(firstBinding) {}
    explicit BindVertexBuffers(std::ranges::range auto&& vertexBuffers, const uint32_t firstBinding=0) :
        Command({}, StateChange),
        buffers(std::ranges::to<std::vector>(vertexBuffers)),
        offsets(buffers.size(), 0),
        firstBinding(firstBinding) {}
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    std::vector<Buffer*> buffers;
    std::vector<VkDeviceSize> offsets;
    uint32_t firstBinding;
  };

  struct BlitImageToImage final : Command {
    template<std::ranges::range T = std::span<VkImageBlit>>
    BlitImageToImage(const Image* const source, const Image* const destination, T&& regions=T{}, const VkFilter filter=VK_FILTER_NEAREST) :
        Command({ResourceAccess{ResourceAccess::Read, source, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
                 ResourceAccess{ResourceAccess::Write, destination, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}
        }, Copy),
        src(source),
        dst(destination),
        blits(std::ranges::to<std::vector>(regions)),
        filter(filter) {
      if (blits.empty()) blits.push_back({
        .srcSubresource = VkImageSubresourceLayers{source->getAspect(), 0, 0, source->getLayerCount()},
        .srcOffsets = {{0, 0, 0}, VkOffset3D{static_cast<int32_t>(source->getExtent().width), static_cast<int32_t>(source->getExtent().height), static_cast<int32_t>(source->getExtent().depth)}},
        .dstSubresource = VkImageSubresourceLayers{destination->getAspect(), 0, 0, destination->getLayerCount()},
        .dstOffsets = {{0, 0, 0}, VkOffset3D{static_cast<int32_t>(destination->getExtent().width), static_cast<int32_t>(destination->getExtent().height), static_cast<int32_t>(destination->getExtent().depth)}},
      });
    }
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Image* src;
    const Image* dst;
    VkImageLayout srcImageLayout{};
    VkImageLayout dstImageLayout{};
    std::vector<VkImageBlit> blits{};
    VkFilter filter{};
  };

  struct ClearColorImage final : Command {
    template<std::ranges::range T = std::span<VkImageSubresourceRange>>
    explicit ClearColorImage(const Image* const image, const VkClearColorValue value={}, T&& subresourceRanges=T{}) :
        Command({ResourceAccess{ResourceAccess::Write, image, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}, Copy),
        image(image),
        value(value),
        ranges(std::ranges::to<std::vector>(subresourceRanges)) {
      if (ranges.empty()) ranges.push_back({image->getAspect(), 0, image->getMipLevels(), 0, image->getLayerCount()});
    }
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Image* image;
    VkClearColorValue value;
    std::vector<VkImageSubresourceRange> ranges;
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct ClearDepthStencilImage final : Command {
    template<std::ranges::range T = std::span<VkImageSubresourceRange>>
    explicit ClearDepthStencilImage(const Image* const image, const VkClearDepthStencilValue value={1}, T&& subresourceRanges=T{}) :
        Command({ResourceAccess{ResourceAccess::Write, image, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}, Copy),
        image(image),
        value(value),
        ranges(std::ranges::to<std::vector>(subresourceRanges)) {
      if (ranges.empty()) ranges.push_back({image->getAspect(), 0, image->getMipLevels(), 0, image->getLayerCount()});
    }
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Image* image;
    VkClearDepthStencilValue value;
    std::vector<VkImageSubresourceRange> ranges;
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct CopyBufferToBuffer final : Command {
    template<std::ranges::range T = std::span<VkBufferCopy>>
    CopyBufferToBuffer(const Buffer* const source, const Buffer* const destination, T&& regions=T{}) :
        Command({ResourceAccess{ResourceAccess::Read, source, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT},
             ResourceAccess{ResourceAccess::Write, destination, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}},
             Copy),
        src(source),
        dst(destination),
        copies(std::ranges::to<std::vector>(regions)) {
      if (copies.empty()) copies.push_back({
        .srcOffset = 0,
        .dstOffset = 0,
        .size = std::min(source->getSize(), destination->getSize())
      });
    }
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Buffer* src;
    const Buffer* dst;
    std::vector<VkBufferCopy> copies;
  };

  struct CopyBufferToImage final : Command {
    template<std::ranges::range T = std::span<VkBufferImageCopy>>
    CopyBufferToImage(const Buffer* const source, const Image* const destination, T&& regions=T{}) :
        Command({ResourceAccess{ResourceAccess::Read, source, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT},
                 ResourceAccess{ResourceAccess::Write, destination, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}},
                 Copy),
        src(source),
        dst(destination),
        copies{std::ranges::to<std::vector<VkBufferImageCopy>>(regions)} {
      if (copies.empty()) {
        const VkExtent3D texelBlockExtent = vkuFormatTexelBlockExtent(destination->getFormat());
        copies.push_back({
          .bufferOffset = 0,
          .bufferRowLength = texelBlockExtent.width * destination->getExtent().width,
          .bufferImageHeight = texelBlockExtent.height * destination->getExtent().height,
          .imageSubresource = VkImageSubresourceLayers{destination->getAspect(), 0, 0, destination->getLayerCount()},
          .imageOffset = VkOffset3D{0, 0, 0},
          .imageExtent = destination->getExtent()
        });
      }
    }
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Buffer* src;
    const Image* dst;
    std::vector<VkBufferImageCopy> copies;
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct CopyImageToBuffer final : Command {
    template<std::ranges::range T = std::span<VkBufferImageCopy>>
    CopyImageToBuffer(const Image* const source, const Buffer* const destination, T&& regions=T{}) :
        Command({ResourceAccess{ResourceAccess::Read, source, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
             ResourceAccess{ResourceAccess::Write, destination, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT}},
             Copy),
        src(source),
        dst(destination),
        copies(std::ranges::to<std::vector>(regions)) {
      if (copies.empty()) {
        const VkExtent3D texelBlockExtent = vkuFormatTexelBlockExtent(source->getFormat());
        copies.push_back({
          .bufferOffset = 0,
          .bufferRowLength = texelBlockExtent.width * source->getExtent().width,
          .bufferImageHeight = texelBlockExtent.height * source->getExtent().height,
          .imageSubresource = VkImageSubresourceLayers{source->getAspect(), 0, 0, source->getLayerCount()},
          .imageOffset = VkOffset3D{0, 0, 0},
          .imageExtent = source->getExtent()
        });
      }
    }
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Image* src;
    const Buffer* dst;
    std::vector<VkBufferImageCopy> copies;
    VkImageLayout layout{VK_IMAGE_LAYOUT_MAX_ENUM};
  };

  struct CopyImageToImage final : Command {
    template<std::ranges::range T = std::span<VkImageCopy>>
    CopyImageToImage(const Image* const src, const Image* const dst, T&& regions=T{}) :
        Command(src == dst ? std::vector{ResourceAccess{ResourceAccess::Read, src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
                                         ResourceAccess{ResourceAccess::Write, dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}}
                           : std::vector{ResourceAccess{ResourceAccess::Read, src, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}},
                                         ResourceAccess{ResourceAccess::Write, dst, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR}}},
                Copy),
        src(src),
        dst(dst),
        copies(std::ranges::to<std::vector>(regions)) {
      if (copies.empty()) {
        copies.push_back({
          .srcSubresource = VkImageSubresourceLayers{src->getAspect(), 0, 0, src->getLayerCount()},
          .srcOffset      = VkOffset3D{0, 0, 0},
          .dstSubresource = VkImageSubresourceLayers{dst->getAspect(), 0, 0, dst->getLayerCount()},
          .dstOffset      = VkOffset3D{0, 0, 0},
          .extent         = VkExtent3D{std::min(src->getExtent().width, dst->getExtent().width), std::min(src->getExtent().height, dst->getExtent().height), std::min(src->getExtent().depth, dst->getExtent().depth)}
        });
      }
    }
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Image* src;
    const Image* dst;
    std::vector<VkImageCopy> copies;
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
    explicit DrawIndexed(uint32_t instanceCount=1);
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    uint32_t instanceCount{0};
    uint32_t vertexCount{0};
    uint32_t indexCount{0};
  };

  struct DrawIndexedIndirect final : Command {
    explicit DrawIndexedIndirect(const Buffer* buffer, uint32_t count=std::numeric_limits<uint32_t>::max(), VkDeviceSize offset=0, uint32_t stride=sizeof(VkDrawIndexedIndirectCommand));
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    const Buffer* buffer;
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
    struct MemoryBarrier {
      VkStructureType sType;
      const void*     pNext;
      VkAccessFlags   srcAccessMask;
      VkAccessFlags   dstAccessMask;
      explicit operator VkMemoryBarrier() const { return {sType, pNext, srcAccessMask, dstAccessMask}; }
    };
    struct BufferMemoryBarrier {
      VkStructureType sType;
      const void*     pNext;
      VkAccessFlags   srcAccessMask;
      VkAccessFlags   dstAccessMask;
      uint32_t        srcQueueFamilyIndex;
      uint32_t        dstQueueFamilyIndex;
      const Buffer*   buffer;
      VkDeviceSize    offset;
      VkDeviceSize    size;
      explicit operator VkBufferMemoryBarrier() const { return {sType, pNext, srcAccessMask, dstAccessMask, srcQueueFamilyIndex, dstQueueFamilyIndex, buffer->getBuffer(), offset, size}; }
    };
    struct ImageMemoryBarrier {
      VkStructureType         sType;
      const void*             pNext;
      VkAccessFlags           srcAccessMask;
      VkAccessFlags           dstAccessMask;
      VkImageLayout           oldLayout;
      VkImageLayout           newLayout;
      uint32_t                srcQueueFamilyIndex;
      uint32_t                dstQueueFamilyIndex;
      const Image*            image;
      VkImageSubresourceRange subresourceRange;
      explicit operator VkImageMemoryBarrier() const { return {sType, pNext, srcAccessMask, dstAccessMask, oldLayout, newLayout, srcQueueFamilyIndex, dstQueueFamilyIndex, image->getImage(), subresourceRange}; }
    };
    PipelineBarrier(const VkPipelineStageFlags srcStageMask, const VkPipelineStageFlags dstStageMask, const VkDependencyFlags dependencyFlags, std::ranges::range auto&& memoryBarriers, std::ranges::range auto&& bufferMemoryBarriers, std::ranges::range auto&& imageMemoryBarriers) :
      Command({}, Synchronization),
      srcStageMask(srcStageMask),
      dstStageMask(dstStageMask),
      dependencyFlags(dependencyFlags),
      memoryBarriers(std::ranges::to<std::vector>(memoryBarriers)),
      bufferMemoryBarriers(std::ranges::to<std::vector>(bufferMemoryBarriers)),
      imageMemoryBarriers(std::ranges::to<std::vector>(imageMemoryBarriers)) {}
  private:
    void preprocess(State& state, PreprocessingFlags flags) override;
    void bake(VkCommandBuffer commandBuffer) override;
    std::string toString(bool includeArguments) override;

    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkDependencyFlags dependencyFlags;
    std::vector<MemoryBarrier> memoryBarriers;
    std::vector<BufferMemoryBarrier> bufferMemoryBarriers;
    std::vector<ImageMemoryBarrier> imageMemoryBarriers;
  };

  template<typename T, typename... Args> requires std::constructible_from<T, Args...> && std::derived_from<T, Command> && (!std::is_same_v<T, Command>) const_iterator record(const const_iterator& iterator, Args&&... args) { return commands.insert(iterator, std::make_unique<T>(std::forward<Args&&>(args)...)); }
  template<typename T, typename... Args> requires std::constructible_from<T, Args...> && std::derived_from<T, Command> && (!std::is_same_v<T, Command>) const_iterator record(Args&&... args) { return commands.insert(commands.cend(), std::make_unique<T>(std::forward<Args&&>(args)...)); }
  void addCleanupResource(Resource* resource);
  /**
   * This command will preprocess this CommandBuffer assuming a set of input states. This process involves not only modifying the recorded commands to be correct with each other, but optimizing them as well.
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

  ~CommandBuffer();
};