#pragma once

#include "DescriptorSetRequirer.hpp"
#include "src/Tools/Hashing.hpp"

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
class MeshGroup;
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

  std::uint64_t frameNumber{};
  std::unique_ptr<VkDescriptorPool, std::function<void(VkDescriptorPool*)>> descriptorPool{VK_NULL_HANDLE};

  std::vector<std::shared_ptr<RenderPass>> renderPasses;
  bool outOfDate = false;

public:
  GraphicsDevice* const device;

  using ImageID = std::uint64_t;
  using ResolutionGroupID = std::uint64_t;

private:
  struct ImageProperties {
    ResolutionGroupID resolutionGroup;
    VkFormat format;
    bool inheritSampleCount;
    Image* image;
    std::string name;  /**@todo: Try to limit the usage of this to just debug builds.*/
  };

  struct ResolutionGroupProperties {
    VkExtent3D resolution;
    VkSampleCountFlags sampleCount;
    plf::colony<ImageID> attachments;
  };

  std::unordered_map<ResolutionGroupID, ResolutionGroupProperties> resolutionGroups;
  std::unordered_map<ImageID, ImageProperties> images;

public:
  static uint8_t FRAMES_IN_FLIGHT;
  
  struct ImageAccess {
    VkImageLayout layout{};
    VkImageUsageFlags usage{};
    VkAccessFlags access{};
    VkPipelineStageFlags stage{};
    VkAttachmentLoadOp loadOp{VK_ATTACHMENT_LOAD_OP_DONT_CARE};
    VkAttachmentStoreOp storeOp{VK_ATTACHMENT_STORE_OP_DONT_CARE};
    VkAttachmentLoadOp stencilLoadOp{VK_ATTACHMENT_LOAD_OP_DONT_CARE};
    VkAttachmentStoreOp stencilStoreOp{VK_ATTACHMENT_STORE_OP_DONT_CARE};
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
  void setResolutionGroup(const std::string_view& name, VkExtent3D resolution, VkSampleCountFlags sampleCount);
  static constexpr ImageID getImageId(const std::string_view name) { return Tools::hash(name); }

  /**
   * Set an attachment's properties. This does <em>not</em> create the actual image. The actual image will be created during baking.
   * @param name The name of the attachment. Pass an empty string to keep the name that the AttachmentID used to have - useful if you have a pre-existing id.
   * @param groupName The resolution group that this attachment should belong to.
   * @param format VkFormat of the image.
   * @param inheritSampleCount Use the sample count of the resolution group. If this is false, the sample count is forced to VK_SAMPLE_COUNT_1_BIT
   * @exception std::out_of_range The `groupName` resolution group does not exist. Use `setResolutionGroup` first.
   */
  void setImage(const std::string_view& name, const std::string_view& groupName, VkFormat format, bool inheritSampleCount);
  [[nodiscard]] ImageProperties getImage(ImageID id) const;
  [[nodiscard]] bool hasImage(ImageID id) const;

  static bool combineImageAccesses(ImageAccess& dst, const ImageAccess& src);

  template<typename T, typename... Args> requires std::constructible_from<T, RenderGraph&, Args...> && std::derived_from<T, RenderPass> && (!std::is_same_v<T, RenderPass>) const_iterator insert(const const_iterator iterator, Args&&... args) { outOfDate = true; return renderPasses.insert(iterator, std::make_unique<T>(*this, std::forward<Args&&>(args)...)); }
  template<typename T, typename... Args> requires std::constructible_from<T, RenderGraph&, Args...> && std::derived_from<T, RenderPass> && (!std::is_same_v<T, RenderPass>) const_iterator insert(Args&&... args) { outOfDate = true; return insert<T>(cend(), std::forward<Args&&>(args)...); }
  bool bake();

  [[nodiscard]] const PerFrameData& getPerFrameData(uint64_t frameIndex=-1) const;
  [[nodiscard]] VkSemaphore waitForNextFrameData() const;
  void update() const;
  /**
   * Executes this RenderGraph then blits the GBufferAlbdeo attachment onto the <c>swapchainImage</c>.
   * @param swapchainImage The image to put the color output onto (usually the swapchain image).
   * @param semaphore The semaphore to signal when the GPU has finished rendering.
   */
  void execute(const std::shared_ptr<Image>& swapchainImage, VkSemaphore semaphore);

  static constexpr std::string_view VoidResolutionGroup = "void";
  static constexpr std::string_view VoidImage           = "void";
  static constexpr std::string_view RenderResolution    = "Render";
  static constexpr std::string_view GBufferAlbedo       = "gBufferAlbedo";
  static constexpr std::string_view GBufferPosition     = "gBufferPosition";
  static constexpr std::string_view GBufferNormal       = "gBufferNormal";
  static constexpr std::string_view GBufferDepth        = "gBufferDepth";
  static constexpr std::string_view RenderColor         = "renderColor";
  static constexpr std::string_view ShadowResolution    = "Shadow";
  static constexpr std::string_view ShadowDepth         = "shadowMap";

private:
  /**
   * Builds the images stored in <c>backingImages</c> from <c>attachmentProperties</c>
   */
  void buildImages(const std::unordered_map<ImageID, VkImageUsageFlags>& usages);
};