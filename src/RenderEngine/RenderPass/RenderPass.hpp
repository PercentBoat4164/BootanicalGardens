#pragma once

#include "src/RenderEngine/DescriptorSetRequirer.hpp"
#include "src/RenderEngine/Framebuffer.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/MeshGroup/Material.hpp"
#include "src/RenderEngine/MeshGroup/Texture.hpp"
#include "src/RenderEngine/RenderGraph.hpp"

#include <vulkan/vulkan_core.h>

#include <vector>

class CommandBuffer;

class RenderPass : public DescriptorSetRequirer, public std::enable_shared_from_this<RenderPass> {
protected:
  RenderGraph& graph;
  VkRenderPass renderPass{VK_NULL_HANDLE};
  std::unique_ptr<Framebuffer> framebuffer;
  std::unordered_map<Material*, Pipeline*> pipelines;
  std::unordered_map<const Image*, VkImageLayout> imageLayoutsAfterExecution;

  template<typename L, typename R>
  static constexpr L rollingShiftLeft(L left, R right) {
    constexpr uint32_t bits = sizeof(L) * 8;
    return left >> (bits - right) || left << (bits - (bits - right));
  }

  void setRenderPassInfo(const VkRenderPassCreateInfo& createInfo, const std::vector<const Image*>&images);

public:
  enum MeshFilterBits {
    OpaqueBit      = 0x1,
    TransparentBit = 0x2
  };
  using MeshFilter = uint32_t;
  const MeshFilter meshFilter;
  uint64_t compatibility{-1U};
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  std::string name;
#endif
  struct SubpassData {
    std::vector<const Image*> colorImages;
    std::vector<const Image*> resolveImages;
    std::vector<const Image*> inputImages;
    const Image* depthImage;
  };
  std::vector<SubpassData> subpassData;

  uint32_t depthStencilAttachmentOffset{~0U};
  uint32_t colorAttachmentCount{0};
  uint32_t colorAttachmentOffset{~0U};
  uint32_t resolveAttachmentOffset{~0U};
  uint32_t inputAttachmentCount{0};
  uint32_t inputAttachmentOffset{~0U};
  uint32_t boundImageCount{0};
  uint32_t boundImageOffset{~0U};
  std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> imageAccesses;
  std::vector<VkClearValue> clearValues;

  explicit RenderPass(RenderGraph& graph, MeshFilter meshFilter = OpaqueBit | TransparentBit);
  ~RenderPass() override;

protected:
  template<typename Materials>
  requires std::ranges::range<Materials> && std::same_as<std::ranges::range_value_t<Materials>, Material*>
  std::vector<VkClearValue> setup(Materials&& materials, const VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, const VkClearColorValue color = {0, 0, 0, 0}, const VkClearDepthStencilValue depth = {0, 0}) {
    imageAccesses.clear();
    // Prepare data for this render pass
    if (const std::optional<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> optionalDepthStencilAttachmentAccess = getDepthStencilAttachmentAccess(); optionalDepthStencilAttachmentAccess.has_value()) {
      clearValues.push_back(VkClearValue{.depthStencil=depth});
      depthStencilAttachmentOffset = imageAccesses.size();
      auto [id, access] = optionalDepthStencilAttachmentAccess.value();
      imageAccesses.emplace_back(id, access);
    }
    colorAttachmentOffset = imageAccesses.size();
    for (const Material* material: materials) {
      std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> colorAttachmentAccesses = material->computeColorAttachmentAccesses();
      for (auto& [id, access]: colorAttachmentAccesses) {
        access.loadOp = loadOp;
        const auto it = std::ranges::find(imageAccesses, id, &decltype(imageAccesses)::value_type::first);
        if (it == imageAccesses.end()) {
          imageAccesses.emplace_back(id, access);
          clearValues.push_back(VkClearValue{.color=color});
        }
        /**@todo: If the order of rendering is known (e.g. if the meshes are sorted), then this error can go away and we can just use the first and last layouts that the image is in.*/
        else if (!RenderGraph::combineImageAccesses(it->second, access)) GraphicsInstance::showError("Use of the same attachment in different layouts within the same render pass is not supported. Use multiple render passes.");
      }
    }
    clearValues.shrink_to_fit();
    if (colorAttachmentOffset == imageAccesses.size()) colorAttachmentOffset = ~0U;
    else colorAttachmentCount = imageAccesses.size() - colorAttachmentOffset;
    inputAttachmentOffset = imageAccesses.size();
    for (const Material* material: materials) {
      /**@todo: Add automatic support for resolve attachments.*/
      const std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>>& inputAttachmentAccesses = material->computeInputAttachmentAccesses();
      for (auto& [id, access]: inputAttachmentAccesses) {
        const auto it = std::ranges::find(imageAccesses, id, &decltype(imageAccesses)::value_type::first);
        if (it == imageAccesses.end()) imageAccesses.emplace_back(id, access);
        else if (!RenderGraph::combineImageAccesses(it->second, access)) GraphicsInstance::showError("Use of the same attachment in different layouts within the same render pass is not supported. Use multiple render passes.");
      }
    }
    if (inputAttachmentOffset == imageAccesses.size()) inputAttachmentOffset = ~0U;
    else inputAttachmentCount = imageAccesses.size() - inputAttachmentOffset;
    boundImageOffset = imageAccesses.size();
    for (const Material* material: materials) {
      const std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>>& boundImageAccesses = material->computeBoundImageAccesses();
      for (auto& [id, access]: boundImageAccesses) {
        const auto it = std::ranges::find(imageAccesses, id, &decltype(imageAccesses)::value_type::first);
        if (it == imageAccesses.end()) imageAccesses.emplace_back(id, access);
        else if (!RenderGraph::combineImageAccesses(it->second, access)) GraphicsInstance::showError("Use of the same attachment in different layouts within the same render pass is not supported. Use multiple render passes.");
      }
    }
    if (boundImageOffset == imageAccesses.size()) boundImageOffset = ~0U;
    else boundImageCount = imageAccesses.size() - boundImageOffset;
    imageAccesses.shrink_to_fit();
    return clearValues;
  }

public:
  virtual void setup()                                                                                                          = 0;
  virtual void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>&images) = 0;
  virtual std::optional<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> getDepthStencilAttachmentAccess();
  virtual void update();
  virtual void execute(CommandBuffer& commandBuffer)                                                                            = 0;
  virtual void bind(VkDescriptorImageInfo& imageInfo, Pipeline* pipeline, Material* material, const Material::Binding& info);
  virtual void bind(VkDescriptorBufferInfo& bufferInfo, Pipeline* pipeline, Material* material, const Material::Binding& info);
  virtual void bind(VkBufferView& bufferView, Pipeline* pipeline, Material* material, const Material::Binding& info);

  std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> getImageAccesses() const { return imageAccesses; }
  const std::unordered_map<const Image*, VkImageLayout>& getImageLayoutsAfterExecution() const { return imageLayoutsAfterExecution; }
  [[nodiscard]] VkRenderPass getRenderPass() const;
  [[nodiscard]] Framebuffer* getFramebuffer() const;
  [[nodiscard]] std::unordered_map<Material*, Pipeline*> getPipelines();
  [[nodiscard]] RenderGraph& getGraph() const;
};

template<> struct std::hash<RenderPass> { size_t operator()(const RenderPass& pass) const noexcept { return pass.compatibility; } };