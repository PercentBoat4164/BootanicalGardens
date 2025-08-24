#pragma once

#include "DescriptorSetRequirer.hpp"
#include "src/RenderEngine/Renderable/Instance.hpp"
#include "src/Tools/StringHash.h"

#include <plf_colony.h>

#include <vulkan/vulkan.h>

#include <functional>
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
    GraphicsDevice* const device;
    const RenderGraph& graph;

  public:
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    VkSemaphore frameFinishedSemaphore{VK_NULL_HANDLE};
    VkSemaphore frameDataSemaphore{VK_NULL_HANDLE};
    VkFence renderFence{VK_NULL_HANDLE};
    std::shared_ptr<VkDescriptorSet> descriptorSet{VK_NULL_HANDLE};
    std::shared_ptr<VkDescriptorSetLayout> descriptorSetLayout{VK_NULL_HANDLE};

    PerFrameData(GraphicsDevice* device, const RenderGraph& graph);
    ~PerFrameData();
  };
  
  std::vector<PerFrameData> frames;
  plf::colony<Instance> instances;

  uint64_t frameNumber{};
  std::unique_ptr<VkDescriptorPool, std::function<void(VkDescriptorPool*)>> descriptorPool{VK_NULL_HANDLE};

  std::vector<std::shared_ptr<RenderPass>> renderPasses;
  bool outOfDate = false;

public:
  GraphicsDevice* const device;

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
  std::unordered_map<AttachmentID, std::string> attachmentNames;

public:
  static uint8_t FRAMES_IN_FLIGHT;
  
  struct AttachmentDeclaration {
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

  explicit RenderGraph(GraphicsDevice* device);
  ~RenderGraph();

  [[nodiscard]] uint64_t getFrameIndex() const { return frameNumber % FRAMES_IN_FLIGHT; }
  static constexpr ResolutionGroupID getResolutionGroupId(const std::string_view name) { return Tools::hash(name); }
  void setResolutionGroup(ResolutionGroupID id, VkExtent3D resolution);
  static constexpr AttachmentID getAttachmentId(const std::string_view name) { return Tools::hash(name); }

  /**
   * Set an attachment's properties. This does <em>not</em> create the actual image. The actual image will be created during baking.
   * @param id The AttachmentID to use for this attachment. Generate this using <c>RenderGraph::getAttachmentId(name)</c>.
   * @param name The name of the attachment. Pass an empty string to keep the name that the AttachmentID used to have - useful if you have a pre-existing id.
   * @param groupId The resolution group that this attachment should belong to.
   * @param format VkFormat of the image.
   * @param sampleCount MSAA sample count of the image. Must be a power of 2. Must not be larger than the maximum number of samples supported by the device.
   */
  void setAttachment(AttachmentID id, const std::string_view& name, ResolutionGroupID groupId, VkFormat format, VkSampleCountFlags sampleCount);
  [[nodiscard]] std::shared_ptr<Image> getAttachmentImage(AttachmentID id) const;
  template<typename T, typename... Args> requires std::constructible_from<T, RenderGraph&, Args...> && std::derived_from<T, RenderPass> && (!std::is_same_v<T, RenderPass>) const_iterator insert(const const_iterator iterator, Args&&... args) { outOfDate = true; return renderPasses.insert(iterator, std::make_unique<T>(*this, std::forward<Args&&>(args)...)); }
  template<typename T, typename... Args> requires std::constructible_from<T, RenderGraph&, Args...> && std::derived_from<T, RenderPass> && (!std::is_same_v<T, RenderPass>) const_iterator insert(Args&&... args) { outOfDate = true; return insert<T>(cend(), std::forward<Args&&>(args)...); }
  bool bake(CommandBuffer& commandBuffer);

  [[nodiscard]] const PerFrameData& getPerFrameData(uint64_t frameIndex=-1) const;
  [[nodiscard]] VkSemaphore waitForNextFrameData() const;
  void update() const;
  /**
   * Executes this RenderGraph then blits the GBufferAlbdeo attachment onto the <c>swapchainImage</c>.
   * @param swapchainImage The image to put the color output onto (usually the swapchain image).
   * @param semaphore The semaphore to signal when the GPU has finished rendering.
   */
  void execute(const std::shared_ptr<Image>& swapchainImage, VkSemaphore semaphore);

  inline static const AttachmentID GBufferAlbedo = getAttachmentId("GBufferAlbdeo");
  inline static const AttachmentID GBufferPosition = getAttachmentId("GBufferPosition");
  inline static const AttachmentID GBufferDepth = getAttachmentId("GBufferDepth");
  inline static const ResolutionGroupID RenderResolution = getResolutionGroupId("Render");

private:
  /**
   * Builds the images stored in <c>backingImages</c> from <c>attachmentProperties</c>
   */
  void buildImages(const std::unordered_map<AttachmentID, VkImageUsageFlags>&usages);

  /**
   * Transitions images in the <c>backingImages</c>
   * @param commandBuffer The CommandBuffer to record transitions into
   * @param declarations Declarations of the images in the <c>backingImages</c>
   */
  void transitionImages(CommandBuffer&commandBuffer, const std::unordered_map<AttachmentID, AttachmentDeclaration>&declarations);
};