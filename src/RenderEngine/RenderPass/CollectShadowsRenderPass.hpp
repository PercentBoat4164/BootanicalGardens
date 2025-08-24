#pragma once
#include "RenderPass.hpp"

template<typename T> class UniformBuffer;

class CollectShadowsRenderPass : public RenderPass {
  struct PassData {
    glm::mat4 view_ViewMatrixInverse;
    glm::mat4 view_ProjectionMatrixInverse;
  };
  std::shared_ptr<UniformBuffer<PassData>> uniformBuffer;
  std::shared_ptr<Material> material;

public:
  explicit CollectShadowsRenderPass(RenderGraph& graph);

  std::vector<std::pair<RenderGraph::AttachmentID, RenderGraph::AttachmentDeclaration>> declareAttachments() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>&) override;
  void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph&graph) override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;
};