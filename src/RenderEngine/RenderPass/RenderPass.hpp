#pragma once

#include "src/RenderEngine/DescriptorSetRequirer.hpp"
#include "src/RenderEngine/Framebuffer.hpp"
#include "src/RenderEngine/RenderGraph.hpp"

#include <map>
#include <vulkan/vulkan_core.h>

#include <vector>

class CommandBuffer;

class RenderPass : public DescriptorSetRequirer, public std::enable_shared_from_this<RenderPass> {
protected:
  RenderGraph& graph;
  VkRenderPass renderPass{VK_NULL_HANDLE};
  std::shared_ptr<Framebuffer> framebuffer;

  std::unordered_map<std::shared_ptr<Material>, std::shared_ptr<Pipeline>> pipelines;

  template<typename L, typename R>
  static constexpr L rollingShiftLeft(L left, R right) {
    constexpr uint32_t bits = sizeof(L) * 8;
    return left >> (bits - right) || left << (bits - (bits - right));
  }

  void setRenderPassInfo(const VkRenderPassCreateInfo&createInfo, const std::vector<std::shared_ptr<Image>>&images);

public:
  enum MeshFilterBits {
    OpaqueBit      = 0x1,
    TransparentBit = 0x2
  };
  using MeshFilter = uint32_t;
  const MeshFilter meshFilter;
  uint64_t compatibility{-1U};
  struct SubpassData {
    std::vector<std::shared_ptr<const Image>> colorImages;
    std::vector<std::shared_ptr<const Image>> resolveImages;
    std::vector<std::shared_ptr<const Image>> inputImages;
    std::shared_ptr<const Image> depthImage;
  };
  std::vector<SubpassData> subpassData;

  explicit RenderPass(RenderGraph& graph, MeshFilter meshFilter = OpaqueBit | TransparentBit);
  ~RenderPass() override;

  virtual std::vector<std::pair<RenderGraph::AttachmentID, RenderGraph::AttachmentDeclaration>> declareAttachments()                       = 0;
  virtual void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) = 0;
  virtual void update()                                                                                                                    = 0;
  virtual void execute(CommandBuffer& commandBuffer)                                                                                       = 0;

  [[nodiscard]] VkRenderPass getRenderPass() const;
  [[nodiscard]] std::shared_ptr<Framebuffer> getFramebuffer() const;
  [[nodiscard]] const std::unordered_map<std::shared_ptr<Material>, std::shared_ptr<Pipeline>>& getPipelines();
  [[nodiscard]] const RenderGraph& getGraph() const;
};

template<> struct std::hash<RenderPass> { size_t operator()(const RenderPass& pass) const noexcept { return pass.compatibility; } };