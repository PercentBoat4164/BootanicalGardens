#pragma once

#include "src/RenderEngine/RenderGraph.hpp"
#include "src/RenderEngine/Resources/Buffer.hpp"

#include <glm/matrix.hpp>
#include <yyjson.h>

class Buffer;
class CommandBuffer;
class GraphicsDevice;
class Material;
template<typename T> class UniformBuffer;

class Mesh {
public:
  struct InstanceCollection {
    struct PerInstanceData {
      glm::mat4 originalModelMatrix;
    };
    plf::colony<glm::mat4> modelInstances;
    plf::colony<uint32_t> materialInstances;
    plf::colony<PerInstanceData> perInstanceData;
    std::unique_ptr<Buffer> modelInstanceBuffer{nullptr};
    std::unique_ptr<Buffer> materialInstanceBuffer{nullptr};
    bool stale = true;
  };

  struct InstanceReference {
    Material* material;
    decltype(InstanceCollection::modelInstances)::iterator modelInstanceID;
    decltype(InstanceCollection::materialInstances)::iterator materialInstanceID;
    decltype(InstanceCollection::perInstanceData)::iterator perInstanceDataID;
  };

  VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_MAX_ENUM};
  GraphicsDevice* device;
  std::unordered_map<Material*, InstanceCollection> instances;
  std::unique_ptr<Buffer> positionsVertexBuffer{nullptr};
  std::unique_ptr<Buffer> textureCoordinatesVertexBuffer{nullptr};
  std::unique_ptr<Buffer> normalsVertexBuffer{nullptr};
  std::unique_ptr<Buffer> tangentsVertexBuffer{nullptr};
  std::unique_ptr<Buffer> indexBuffer{nullptr};
  bool stale = true;

  Mesh(GraphicsDevice* device, yyjson_val* json);
  ~Mesh();

  InstanceReference addInstance(uint64_t materialID, glm::mat4 mat);
  void removeInstance(InstanceReference&& instanceReference);

  void update(CommandBuffer& commandBuffer);
};
