#pragma once

#include "src/Tools/StringHash.h"

#include <plf_colony.h>

#include <vulkan/vulkan.h>

#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>


class Buffer;
class Material;
class Mesh;
class CommandBuffer;
class GraphicsDevice;
class Pipeline;
class RenderPass;
class Image;
class Renderable;
template <typename T> class UniformBuffer;

class RenderGraph {
  struct GraphData {
    uint32_t frameNumber;
    float time;
  };

  std::shared_ptr<UniformBuffer<GraphData>> uniformBuffer{};

  class PerFrameData {
    const std::shared_ptr<GraphicsDevice> device;
    const RenderGraph& graph;

  public:
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    VkSemaphore frameFinishedSemaphore{VK_NULL_HANDLE};
    VkSemaphore frameDataSemaphore{VK_NULL_HANDLE};
    VkFence renderFence{VK_NULL_HANDLE};
    std::shared_ptr<VkDescriptorSet> descriptorSet{VK_NULL_HANDLE};

    PerFrameData(const std::shared_ptr<GraphicsDevice>& device, const RenderGraph& graph);

    ~PerFrameData();
  };
  
  std::vector<PerFrameData> frames;

  uint64_t frameNumber{};
  std::unique_ptr<VkDescriptorPool, std::function<void(VkDescriptorPool*)>> descriptorPool{VK_NULL_HANDLE};

  std::vector<std::shared_ptr<RenderPass>> renderPasses;
  plf::colony<std::shared_ptr<Renderable>> renderables;
  bool outOfDate = false;

public:
  const std::shared_ptr<GraphicsDevice> device;

  using AttachmentID = uint32_t;
  using ResolutionGroupID = uint32_t;

private:
  struct AttachmentProperties {
    ResolutionGroupID resolutionGroup;
    VkFormat format;
    VkSampleCountFlags sampleCount;
  };

  std::unordered_map<ResolutionGroupID, std::tuple<VkExtent3D, std::vector<AttachmentID>>> resolutionGroups;
  std::unordered_map<AttachmentID, AttachmentProperties> attachmentsProperties;
  std::unordered_map<AttachmentID, std::shared_ptr<Image>> backingImages;

public:
  static uint8_t FRAMES_IN_FLIGHT;
  
  struct AttachmentDeclaration {
    /**@todo: Replace the 4 items below with one or more custom ones. This would allow easier specification of the render pass attachments.*/
    VkImageLayout layout{};
    VkImageUsageFlags usage{};
    VkAccessFlags access{};
    VkPipelineStageFlags stage{};
    VkAttachmentLoadOp loadOp{};
    VkAttachmentStoreOp storeOp{};
    VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  };

  using iterator = decltype(renderPasses)::iterator;
  using reverse_iterator = decltype(renderPasses)::reverse_iterator;
  using const_iterator = decltype(renderPasses)::const_iterator;
  using const_reverse_iterator = decltype(renderPasses)::const_reverse_iterator;

  iterator begin() { return renderPasses.begin(); }
  iterator end() { return renderPasses.end(); }
  reverse_iterator rbegin() { return renderPasses.rbegin(); }
  reverse_iterator rend() { return renderPasses.rend(); }
  [[nodiscard]] const_iterator cbegin() const { return renderPasses.cbegin(); }
  [[nodiscard]] const_iterator cend() const { return renderPasses.cend(); }
  [[nodiscard]] const_reverse_iterator crbegin() const { return renderPasses.crbegin(); }
  [[nodiscard]] const_reverse_iterator crend() const { return renderPasses.crend(); }

  explicit RenderGraph(const std::shared_ptr<GraphicsDevice>& device);
  ~RenderGraph();

  [[nodiscard]] uint64_t getFrameIndex() const { return frameNumber % FRAMES_IN_FLIGHT; }
  static constexpr ResolutionGroupID getResolutionGroupId(const std::string_view name) { return Tools::hash(name); }
  void setResolutionGroup(ResolutionGroupID id, VkExtent3D resolution);
  static constexpr AttachmentID getAttachmentId(const std::string_view name) { return Tools::hash(name); }
  void setAttachment(AttachmentID id, ResolutionGroupID groupId, VkFormat format, VkSampleCountFlags sampleCount);
  void addRenderable(const std::shared_ptr<Renderable>& renderable);
  void removeRenderable(const std::shared_ptr<Renderable>& renderable);
  template<typename T, typename... Args> requires std::constructible_from<T, RenderGraph&, Args...> && std::derived_from<T, RenderPass> && (!std::is_same_v<T, RenderPass>) const_iterator insert(const const_iterator iterator, Args&&... args) { outOfDate = true; return renderPasses.insert(iterator, std::make_unique<T>(*this, std::forward<Args&&>(args)...)); }
  template<typename T, typename... Args> requires std::constructible_from<T, RenderGraph&, Args...> && std::derived_from<T, RenderPass> && (!std::is_same_v<T, RenderPass>) const_iterator insert(Args&&... args) { outOfDate = true; return insert<T>(cend(), std::forward<Args&&>(args)...); }
  bool bake(CommandBuffer& commandBuffer);

  [[nodiscard]] const PerFrameData& getPerFrameData(uint64_t frameIndex=-1) const;
  [[nodiscard]] VkSemaphore waitForNextFrameData() const;
  void update() const;
  [[nodiscard]] VkSemaphore execute(const std::shared_ptr<Image>& swapchainImage) const;

  inline static const AttachmentID RenderColor = getAttachmentId("RenderColor");
  inline static const AttachmentID RenderDepth = getAttachmentId("RenderDepth");
  inline static const ResolutionGroupID RenderResolution = getResolutionGroupId("Render");

private:
  /**
   * Builds the images stored in `backingImages` from `attachmentProperties`
   */
  void buildImages();

  /**
   * Transitions images in the `backingImages`
   * @param commandBuffer The CommandBuffer to record transitions into
   * @param declarations Declarations of the images in the `backingImages`
   */
  void transitionImages(CommandBuffer& commandBuffer, const std::map<AttachmentID, AttachmentDeclaration>& declarations);
};