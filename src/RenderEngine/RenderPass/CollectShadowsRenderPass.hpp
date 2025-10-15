#pragma once

#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/Shader.hpp"

#include "glm/matrix.hpp"

template<typename T> class UniformBuffer;

class CollectShadowsRenderPass : public RenderPass {
  struct PassData {
    glm::mat4 view_ViewMatrixInverse;
    glm::mat4 view_ProjectionMatrixInverse;
  };
  std::unique_ptr<UniformBuffer<PassData>> uniformBuffer;
  Shader* vertexShaderOverride = nullptr;
  static constexpr std::string_view PassName = "Collect Shadows Render Pass";

public:
  explicit CollectShadowsRenderPass(RenderGraph& graph);

  std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> declareAccesses() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>&) override;
  void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph&graph) override;

  std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> getDepthStencilAttachmentAccess() override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;
};