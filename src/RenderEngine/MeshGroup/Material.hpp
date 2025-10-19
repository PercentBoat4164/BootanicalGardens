#pragma once

#include "src/RenderEngine/RenderGraph.hpp"
#include "src/RenderEngine/Shader.hpp"

#include <fastgltf/core.hpp>


class Shader;
class Pipeline;
class Texture;
class GraphicsDevice;
class CommandBuffer;

class Material {
public:
  struct Binding {
    uint64_t nameHash;
    VkDescriptorType type;
    uint32_t count;
#if BOOTANICAL_GARDENS_ENABLE_READABLE_SHADER_VARIABLE_NAMES
    std::string name;
#endif
  };

  GraphicsDevice* device;

  Shader* vertexShader = nullptr;
  Shader* fragmentShader = nullptr;

  // A map of shader bindings for the material set to hashes of the names
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, Binding>> perSetBindings;
  std::string name;

  bool doubleSided = false;
  fastgltf::AlphaMode alphaMode = fastgltf::AlphaMode::Opaque;
  float alphaCutoff = 0;

  std::weak_ptr<Texture> albedoTexture;
  std::weak_ptr<Texture> normalTexture;

  const std::uint32_t id;

  Material(GraphicsDevice* device, yyjson_val* json, std::uint32_t id);

  [[nodiscard]] const std::unordered_map<uint32_t, Binding>* getBindings(uint8_t set) const;

  /**@todo: Cache the values from each of these functions.*/
  [[nodiscard]] std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> computeColorAttachmentAccesses() const;
  [[nodiscard]] std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> computeInputAttachmentAccesses() const;
  [[nodiscard]] std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> computeBoundImageAccesses() const;

  void computeDescriptorSetRequirements(std::map<DescriptorSetRequirer*, std::vector<VkDescriptorSetLayoutBinding>>&requirements, RenderPass* renderPass, Pipeline* pipeline);

  [[nodiscard]] std::vector<VkVertexInputBindingDescription> computeVertexBindingDescriptions() const;
  [[nodiscard]] std::vector<VkVertexInputAttributeDescription> computeVertexAttributeDescriptions() const;
  [[nodiscard]] std::vector<VkPushConstantRange> computePushConstantRanges() const;

  Material* getVertexVariation(Shader* shader) const;
  Material* getFragmentVariation(Shader* shader) const;
  std::uint32_t getId() const;
};