#pragma once

#include "src/RenderEngine/RenderPass/RenderPass.hpp"

#include <glm/matrix.hpp>
#include <glm/vec4.hpp>

class SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass : public RenderPass {
  struct PassData {
    glm::vec4 resolution;
    glm::mat4 viewProj;
    glm::mat4 prevViewProj;
    glm::mat4 guiOrtho;
    struct SMAAParameters {
      float threshold;
      float depthThreshold;
      uint  maxSearchSteps;
      uint  maxSearchStepsDiag;

      uint  cornerRounding;
      uint  pad0;
      uint  pad1;
      uint  pad2;
    } parameters;
    glm::vec4 subsampleIndices;

    float predicationThreshold;
    float predicationScale;
    float predicationStrength;
    float reprojWeigthScale;
  };
  std::unique_ptr<UniformBuffer<PassData>> uniformBuffer;

public:
  explicit SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass(RenderGraph& graph);

  void setup() override;
  void bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>& images) override;
  void writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) override;
  void update() override;
  void execute(CommandBuffer& commandBuffer) override;

  Material* material;
};
