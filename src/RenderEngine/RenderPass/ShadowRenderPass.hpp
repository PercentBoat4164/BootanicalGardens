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

  FragmentProcess* fragmentProcessOverride;

public:
  explicit ShadowRenderPass(RenderGraph& graph);

  void setup() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>&) override;
  void writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) override;
  std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> getDepthStencilAttachmentAccess() override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;
  void bind(VkDescriptorImageInfo& imageInfo, Pipeline* pipeline, Material* material, const Material::Binding& info) override;
  void bind(VkDescriptorBufferInfo& bufferInfo, Pipeline* pipeline, Material* material, const Material::Binding& info) override;
  void bind(VkBufferView& bufferView, Pipeline* pipeline, Material* material, const Material::Binding& info) override;
};
