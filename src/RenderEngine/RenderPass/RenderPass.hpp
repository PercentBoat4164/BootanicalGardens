#pragma once

#include "src/RenderEngine/Framebuffer.hpp"
#include "src/RenderEngine/RenderGraph.hpp"

#include <vulkan/vulkan_core.h>

#include <unordered_set>
#include <vector>

class CommandBuffer;

class RenderPass : public std::enable_shared_from_this<RenderPass> {
protected:
  RenderGraph& graph;
  VkRenderPass renderPass{VK_NULL_HANDLE};
  std::shared_ptr<Framebuffer> framebuffer;
  std::unordered_set<std::shared_ptr<Mesh>> meshes;

  template<typename L, typename R>
  static constexpr L rollingShiftLeft(L left, R right) {
    constexpr uint32_t bits = sizeof(L) * 8;
    return left >> bits - right || left << bits - (bits - right);
  }

  static uint64_t computeRenderPassCompatibility(const VkRenderPassCreateInfo& createInfo);

public:
  enum MeshFilterBits {
    OpaqueBit      = 0x1,
    TransparentBit = 0x2
  };
  using MeshFilter = uint32_t;
  const MeshFilter meshFilter;
  uint64_t compatibility{-1U};

  explicit RenderPass(RenderGraph& graph, MeshFilter meshFilter = OpaqueBit | TransparentBit);
  virtual ~RenderPass();

  void addMesh(const std::shared_ptr<Mesh>& mesh);
  void removeMesh(const std::shared_ptr<Mesh>& mesh);

  virtual std::vector<std::pair<RenderGraph::AttachmentID, RenderGraph::AttachmentDeclaration>> declareAttachments()                = 0;
  virtual void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>&) = 0;
  virtual void update(const RenderGraph& graph)                                                                                     = 0;
  virtual void execute(CommandBuffer& commandBuffer)                                                                                = 0;

  [[nodiscard]] VkRenderPass getRenderPass() const;
  [[nodiscard]] std::shared_ptr<Framebuffer> getFramebuffer() const;
};
