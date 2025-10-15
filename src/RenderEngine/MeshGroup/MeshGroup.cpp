#include "MeshGroup.hpp"

#include "src/Entity.hpp"
#include "src/Tools/Json/Json_glm.hpp"

#include <src/RenderEngine/GraphicsDevice.hpp>

MeshGroup::MeshGroup(const std::uint64_t id, Entity& entity, GraphicsDevice* const device, yyjson_val* val) : Component(id, entity), device(device) {
  yyjson_val* meshesArray = yyjson_obj_get(val, "meshes");
  yyjson_val* materialsArray = yyjson_obj_get(val, "materials");
  yyjson_val* transformationsArray = yyjson_obj_get(val, "transformations");
  const std::uint64_t size = yyjson_arr_size(transformationsArray);
  for (uint64_t i = 0; i < size; ++i) {
    Mesh* mesh = device->getJSONMesh(yyjson_get_uint(yyjson_arr_get(meshesArray, i)));
    meshes[mesh].emplace(mesh->addInstance(yyjson_get_uint(yyjson_arr_get(materialsArray, i)), Tools::jsonGet<glm::mat4>(yyjson_arr_get(transformationsArray, i))));
  }
}

MeshGroup::~MeshGroup() {
  for (auto& [mesh, instances] : meshes) {
    /**@todo: Add a removeInstances function to the Mesh.*/
    for (Mesh::InstanceReference& instance: instances) mesh->removeInstance(std::move(instance));
  }
}

void MeshGroup::onTick() {
  /**@todo: Move me to somewhere in the graphics engine.*/
  // plf::colony<MeshGroup> meshGroups = ECS::getAll<MeshGroup>();
  // for (const MeshGroup& meshGroup: meshGroups) {
  //   const glm::mat4 entityTransform = glm::translate(glm::rotate(glm::scale(glm::identity<glm::mat4>(), meshGroup.entity.scale), glm::angle(meshGroup.entity.rotation), glm::axis(meshGroup.entity.rotation)), meshGroup.entity.position);
    const glm::mat4 entityTransform = glm::translate(glm::rotate(glm::scale(glm::identity<glm::mat4>(), entity.scale), glm::angle(entity.rotation), glm::axis(entity.rotation)), entity.position);
    for (auto& [mesh, references]: meshes) {
      bool& stale = mesh->stale;
      for (const Mesh::InstanceReference& reference: references) {
        glm::mat4 modelMatrix = reference.modelInstanceID->modelMatrix;
        reference.modelInstanceID->modelMatrix = entityTransform * reference.perInstanceDataID->originalModelMatrix;
        stale |= mesh->instances[reference.material].stale |= modelMatrix != reference.modelInstanceID->modelMatrix;
      }
    }
  // }
}