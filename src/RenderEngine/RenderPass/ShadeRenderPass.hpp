#pragma once

#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/Pipeline/Shader.hpp"

#include "glm/matrix.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

template<typename T> class UniformBuffer;

class ShadeRenderPass : public RenderPass {
  struct PassData {
    glm::mat4 inverseViewProjectionMatrix;
    glm::vec3 position;
    float padding;
    glm::vec2 resolution;
  };
  std::unique_ptr<UniformBuffer<PassData>> passData;

  struct LightData {
    glm::mat4 viewProjectionMatrix;
    glm::vec3 position;
  };
  std::unique_ptr<UniformBuffer<LightData>> lightData{nullptr};
  VertexProcess* vertexProcessOverride;
  std::unique_ptr<Buffer> copyBuffer;

  std::unordered_map<Material*, Material*> materialRemap;

public:
  explicit ShadeRenderPass(RenderGraph& graph);

  void setup() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>&) override;
  void writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) override;
  std::optional<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> getDepthStencilAttachmentAccess() override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;
  void bind(VkDescriptorBufferInfo& bufferInfo, Pipeline* pipeline, Material* material, const Material::Binding& info) override;
};