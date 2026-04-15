#pragma once

#include "src/RenderEngine/MeshGroup/Material.hpp"
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
  using InstanceID = std::uint64_t;

  class InstanceGroup {
    Mesh* parent;
    Material* material;

  public:
    InstanceGroup(Mesh* parent, Material* material, const std::size_t initialReservedInstanceCount) : parent(parent), material(material) {
      reservedInstanceCount = initialReservedInstanceCount;

      materialInstanceBuffer = std::make_unique<Buffer>(parent->device, (parent->name + " | Materials").c_str(), sizeof(Material::MaterialID) * reservedInstanceCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
      transformInstanceBuffer = std::make_unique<Buffer>(parent->device, (parent->name + " | Transforms").c_str(), sizeof(glm::mat4) * reservedInstanceCount, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
    }

    std::size_t instanceCount = 0;
    std::size_t reservedInstanceCount = 0;

    std::vector<InstanceID> instanceIds;

    std::unique_ptr<Buffer> transformInstanceBuffer;
    std::unique_ptr<Buffer> materialInstanceBuffer;

  /**
   * @brief Reserves space for a given number of instances.
   *
   * @param count The new maximum possible instance count.
   */
  void reserveInstanceCount(std::size_t count, CommandBuffer& commandBuffer);
  /**
   * @brief Creates a new instance of this Mesh with the given parameters.
   *
   * It is UB to call this function if there is not enough space for the new instance.
   * Ensure that this will not happen before calling this function by checking that <code>instanceCount < reservedInstanceCount</code>.
   * If needed, call <code>reserveInstanceCount(std::size_t)</code> to increase the <code>reservedInstanceCount</code>.
   *
   * @param materialId The ID of the material to assign to the instance
   * @param transform The transform matrix for the instance
   * @param commandBuffer The command buffer to record the instance addition into
   * @return The ID of the added instance
   */
  std::size_t addInstance(Material::MaterialID materialId, const glm::mat4& transform, CommandBuffer& commandBuffer);
  /**todo: Add a `addInstances` function that can add multiple instances at once.*/
  /**
   * @brief Removes an instance of this Mesh.
   *
   * Removal is done by copying the last instance to the removed instance's index.
   *
   * @param index The ID of the instance to remove.
   * @param commandBuffer The CommandBuffer to record the commands that will update the instance buffers into.
   **/
  void removeInstance(size_t index, CommandBuffer& commandBuffer);
  /**todo: Add a `removeInstances` function that can remove multiple instance at once.
   *  - must search for contiguous ranges in the index space of instances marked for removal.
   **/
  /**todo: Add a `prune` function that resizes the instance buffers to exactly the necessary size.*/
  };

  std::unordered_map<Material*, InstanceGroup> instanceGroups;

private:
  InstanceID instanceUuid = 0;
  struct InstanceReference {
    Material* material;
    std::size_t index;
  };
  std::unordered_map<InstanceID, InstanceReference> instanceReferences;

public:
  GraphicsDevice* device;
  std::string name;
  VkPrimitiveTopology topology;

  std::unique_ptr<Buffer> positionsVertexBuffer{nullptr};
  std::unique_ptr<Buffer> textureCoordinatesVertexBuffer{nullptr};
  std::unique_ptr<Buffer> normalsVertexBuffer{nullptr};
  std::unique_ptr<Buffer> tangentsVertexBuffer{nullptr};
  std::unique_ptr<Buffer> indexBuffer{nullptr};

  Mesh(GraphicsDevice* device, yyjson_val* json);

  InstanceGroup& getInstanceGroup(Material* material);
  InstanceID addInstance(Material* material, const glm::mat4& transform, CommandBuffer& commandBuffer);
  void removeInstance(InstanceID id, CommandBuffer& commandBuffer);
  void updateInstance(InstanceID id, Material* material, CommandBuffer& commandBuffer);
  void updateInstance(InstanceID id, const glm::mat4& transform, CommandBuffer& commandBuffer);
};
