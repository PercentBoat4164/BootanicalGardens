#pragma once
#include "RenderPass.hpp"

#include <glm/matrix.hpp>

class Material;
template<typename> class UniformBuffer;

class ShadowRenderPass : public RenderPass {
  struct PassData {
    glm::mat4 light_ViewProjectionMatrix;
  };
  std::unique_ptr<UniformBuffer<PassData>> passData;
  std::shared_ptr<Image> shadowMap;

  FragmentProcess* fragmentProcessOverride;
  std::unordered_map<Material*, Material*> materialRemap;

public:
  explicit ShadowRenderPass(RenderGraph& graph);

  void setup() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>&) override;
  void writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) override;
  std::optional<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> getDepthStencilAttachmentAccess() override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;
};
