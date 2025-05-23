#pragma once

#include "src/RenderEngine/RenderPass/RenderPass.hpp"

#include "glm/mat4x4.hpp"

class Buffer;
template<typename T> class UniformBuffer;

class OpaqueRenderPass final : public RenderPass {
  struct PassData { glm::mat4 viewProjectionMatrix; };
  RenderGraph& graph;
  std::vector<std::shared_ptr<VkDescriptorSet>> descriptorSets;
  std::shared_ptr<UniformBuffer<PassData>> uniformBuffer;

public:
  explicit OpaqueRenderPass(RenderGraph& graph);

  std::vector<std::pair<RenderGraph::AttachmentID, RenderGraph::AttachmentDeclaration>> declareAttachments() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) override;
  void update(const RenderGraph& graph) override;
  void execute (CommandBuffer& commandBuffer) override;
};
