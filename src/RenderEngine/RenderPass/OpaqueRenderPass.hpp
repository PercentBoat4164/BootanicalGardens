#pragma once

#include "RenderPass.hpp"

class Buffer;

class OpaqueRenderPass final : public RenderPass {
  RenderGraph& graph;
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
  std::vector<std::shared_ptr<Buffer>> uniformBuffers;

public:
  explicit OpaqueRenderPass(RenderGraph& graph);

  std::vector<std::pair<RenderGraph::AttachmentID, RenderGraph::AttachmentDeclaration>> declareAttachments() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) override;
  void update(const RenderGraph& graph) override;
  void execute (CommandBuffer& commandBuffer) override;
};
