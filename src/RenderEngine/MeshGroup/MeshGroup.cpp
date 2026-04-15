#include "MeshGroup.hpp"

#include "src/Entity.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/Tools/Json/Json_glm.hpp"

#include <src/RenderEngine/GraphicsDevice.hpp>

MeshGroup::MeshGroup(const std::uint64_t id, Entity& entity, GraphicsDevice* const device, yyjson_val* val) : Component(id, entity), device(device) {
  CommandBuffer commandBuffer;
  yyjson_val* meshesArray = yyjson_obj_get(val, "meshes");
  yyjson_val* materialsArray = yyjson_obj_get(val, "materials");
  yyjson_val* transformationsArray = yyjson_obj_get(val, "transformations");
  const std::uint64_t size = yyjson_arr_size(transformationsArray);
  for (uint64_t i = 0; i < size; ++i) {
    Mesh* mesh = device->getJSONMesh(yyjson_get_uint(yyjson_arr_get(meshesArray, i)));
    Material* const material = device->getJSONObjectMaterial(yyjson_get_uint(yyjson_arr_get(materialsArray, i)));
    meshes[mesh].emplace(mesh->addInstance(material, Tools::jsonGet<glm::mat4>(yyjson_arr_get(transformationsArray, i)), commandBuffer));
  }
  device->executeCommandBufferImmediate(commandBuffer);
}

MeshGroup::~MeshGroup() {
  CommandBuffer commandBuffer;
  for (auto& [mesh, instances] : meshes)
    for (const Mesh::InstanceID& instance: instances) mesh->removeInstance(instance, commandBuffer);
  device->executeCommandBufferImmediate(commandBuffer);
}