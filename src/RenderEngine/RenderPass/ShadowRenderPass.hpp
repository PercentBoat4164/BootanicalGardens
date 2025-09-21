#pragma once
#include "RenderPass.hpp"

class Material;
template<typename> class UniformBuffer;

class ShadowRenderPass : public RenderPass {
  struct PassData {
    glm::mat4 light_ViewProjectionMatrix;
  };
  std::shared_ptr<UniformBuffer<PassData>> uniformBuffer;

  std::shared_ptr<Material> material;

public:
  explicit ShadowRenderPass(RenderGraph& graph);

  std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> declareAccesses() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>&) override;
  void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph&graph) override;

  std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> getDepthStencilAttachmentAccess() override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;
};
