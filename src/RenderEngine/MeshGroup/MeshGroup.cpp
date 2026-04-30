#include "MeshGroup.hpp"

#include "src/Entity.hpp"
#include "src/Tools/Json/Json_glm.hpp"

#include <src/RenderEngine/GraphicsDevice.hpp>

MeshGroup::MeshGroup(GraphicsDevice* const device, yyjson_val* val) noexcept : device(device) {
  yyjson_val* meshesArray = yyjson_obj_get(val, "meshes");
  yyjson_val* materialsArray = yyjson_obj_get(val, "materials");
  yyjson_val* transformationsArray = yyjson_obj_get(val, "transformations");
  const std::uint64_t size = yyjson_arr_size(transformationsArray);
  for (uint64_t i = 0; i < size; ++i) {
    Mesh* mesh = device->getJSONMesh(yyjson_get_uint(yyjson_arr_get(meshesArray, i)));
    meshes[mesh].emplace_back(mesh->addInstance(yyjson_get_uint(yyjson_arr_get(materialsArray, i)), Tools::jsonGet<glm::mat4>(yyjson_arr_get(transformationsArray, i))));
  }
}

void foo(GraphicsDevice* device, const std::vector<yyjson_val*>& val) {
  std::vector<MeshGroup> groups;
  groups.reserve(val.size());
  for (yyjson_val* currVal: val) {
    groups.emplace_back(device, currVal);
  }
  auto yay = std::make_unique<MeshGroupColumn>(std::move(groups));
}
