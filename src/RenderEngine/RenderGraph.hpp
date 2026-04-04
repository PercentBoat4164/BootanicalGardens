#pragma once

#include "src/RenderEngine/GraphicsDevice.hpp"
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
  std::unique_ptr<VkDescriptorPool, std::function<void(VkDescriptorPool*)>> descriptorPool{VK_NULL_HANDLE, [](VkDescriptorPool*){}};

  std::vector<std::shared_ptr<RenderPass>> renderPasses;
  bool outOfDate = false;

public:
  GraphicsDevice* const device;

  struct ImageParameters {
#if !defined(NDEBUG)
    std::string name;
#endif
    std::uint64_t layers;
    std::uint64_t mipLevels;
    VkImageUsageFlags usage;
    VkFormat format;
    VkSampleCountFlags sampleCount;
    VkExtent3D resolution;
  };

private:
  std::unordered_map<GraphicsDevice::ImageID, ImageParameters> imageParameters;
  std::unordered_map<GraphicsDevice::ImageID, std::shared_ptr<Image>> images;

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

  struct Settings {
    VkExtent3D renderResolution{};
  } settings;

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

  [[nodiscard]] std::uint64_t getFrameIndex() const { return frameNumber % FRAMES_IN_FLIGHT; }
  static constexpr GraphicsDevice::ImageID getAttachmentId(const std::string_view& name) { return Tools::hash(name); }
  void setImageParameters(const GraphicsDevice::ImageID id, const ImageParameters& parameters) { imageParameters[id] = parameters; }
  ImageParameters& getImageParameters(const GraphicsDevice::ImageID id) { return imageParameters[id]; }
  void addImageUsageParameters(const GraphicsDevice::ImageID id, const VkImageUsageFlags usage) { imageParameters.at(id).usage |= usage; }
  [[nodiscard]] std::shared_ptr<Image> getImage(GraphicsDevice::ImageID id);

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

  static constexpr GraphicsDevice::ImageID GBufferTextureCoordinate  = Tools::hash("gBufferTextureCoordinate");
  static constexpr GraphicsDevice::ImageID GBufferNormal             = Tools::hash("gBufferNormal");
  static constexpr GraphicsDevice::ImageID GBufferTangent            = Tools::hash("gBufferTangent");
  static constexpr GraphicsDevice::ImageID GBufferMaterialID         = Tools::hash("gBufferMaterialID");
  static constexpr GraphicsDevice::ImageID GBufferDepth              = Tools::hash("gBufferDepth");
  static constexpr GraphicsDevice::ImageID RenderColor               = Tools::hash("renderColor");
  static constexpr GraphicsDevice::ImageID ShadowMap                 = Tools::hash("shadowMap");
};