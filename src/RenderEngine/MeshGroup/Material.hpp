#pragma once

#include "src/RenderEngine/RenderGraph.hpp"
#include "src/RenderEngine/Pipeline/FragmentProcess.hpp"
#include "src/RenderEngine/Pipeline/VertexProcess.hpp"

#include <fastgltf/core.hpp>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct yyjson_val;

class Shader;
class Pipeline;
class Texture;
class GraphicsDevice;
class CommandBuffer;
class DescriptorSetRequirer;

class Material {
public:
  struct Binding {
    GraphicsDevice::ImageID id;
    VkDescriptorType type;
    uint32_t count;
#if defined(BOOTANICAL_GARDENS_ENABLE_READABLE_SHADER_VARIABLE_NAMES)
    std::string name;
#endif
  };

  float id;

  GraphicsDevice* device;

  VertexProcess* vertexProcess;
  FragmentProcess* fragmentProcess;

  // A map of shader bindings for the material set to hashes of the names
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, Binding>> perSetBindings;
  std::string name;

  fastgltf::AlphaMode alphaMode = fastgltf::AlphaMode::Opaque;
  float alphaCutoff = 0;

  Material(GraphicsDevice* device, yyjson_val* json);

  [[nodiscard]] const std::unordered_map<uint32_t, Binding>* getBindings(uint8_t set) const;

  /**@todo: Cache the values from each of these functions.*/
  [[nodiscard]] std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> computeColorAttachmentAccesses() const;
  [[nodiscard]] std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> computeInputAttachmentAccesses() const;
  [[nodiscard]] std::vector<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> computeBoundImageAccesses() const;

  void computeDescriptorSetRequirements(std::map<DescriptorSetRequirer*, std::vector<VkDescriptorSetLayoutBinding>>&requirements, RenderPass* renderPass, Pipeline* pipeline);

  [[nodiscard]] std::vector<VkVertexInputBindingDescription> computeVertexBindingDescriptions() const;
  [[nodiscard]] std::vector<VkVertexInputAttributeDescription> computeVertexAttributeDescriptions() const;
  [[nodiscard]] std::vector<VkPushConstantRange> computePushConstantRanges() const;

  Material* getVertexVariation(VertexProcess* vertexProcess) const;
  Material* getFragmentVariation(FragmentProcess* fragmentProcess) const;

private:
  /**
   * Gets the ID of an image based on its name as used in shaders.
   *
   * @tparam T Any hashable string type.
   * @param name The name of the image as referenced in shaders.
   * @return The ID that can be used to reference this image.
   */
  template<typename T> GraphicsDevice::ImageID getImageID(const T& name) const {
    const GraphicsDevice::ImageID id = Tools::hash(name);
    if (const auto it = JSONTextures.find(id); it != JSONTextures.end()) return it->second;
    return id;
  }

  /**
   * Maps the hash of a texture's name to the ID used to identify it in code.
   * This only applies to textures that are loaded from JSON.
   **/
  std::unordered_map<std::uint64_t, GraphicsDevice::ImageID> JSONTextures;
};