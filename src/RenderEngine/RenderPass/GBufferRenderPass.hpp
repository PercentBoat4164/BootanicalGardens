#pragma once

#include "src/RenderEngine/RenderPass/RenderPass.hpp"

#include "glm/mat4x4.hpp"

class Buffer;
template<typename T> class UniformBuffer;

class GBufferRenderPass final : public RenderPass {
  struct PassData { glm::mat4 view_ViewProjectionMatrix; };
  std::shared_ptr<UniformBuffer<PassData>> uniformBuffer;

public:
  explicit GBufferRenderPass(RenderGraph& graph);

  std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> declareAccesses() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) override;
  void writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph&graph) override;

  std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> getDepthStencilAttachmentAccess() override;
  void update() override;
  void execute (CommandBuffer& commandBuffer) override;
};
