#include "MeshGroup.hpp"

#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/Tools/Json/Json_glm.hpp"

#include <yyjson.h>

void MeshGroup::onTick() {
}

MeshGroup::MeshGroup(const std::uint64_t id, Entity& entity) : Component(id, entity) {}

std::shared_ptr<Component> MeshGroup::create(std::uint64_t id, Entity& entity, yyjson_val* componentData) {
  auto meshGroup = std::make_shared<MeshGroup>(id, entity);
  meshGroup->transformation = Tools::jsonGet<glm::mat4>(componentData, "transformation");
  yyjson_val* meshIndexArray = yyjson_obj_get(componentData, "meshes");
  yyjson_val* materialIndexArray = yyjson_obj_get(componentData, "materials");
  /**@todo: Log this, do not just crash.*/
  assert(yyjson_get_len(meshIndexArray) == yyjson_get_len(materialIndexArray));  // If these do not match, then there is an error in the json by definition.
  meshGroup->meshIndices.resize(yyjson_get_len(meshIndexArray));
  meshGroup->materialIndices.resize(yyjson_get_len(meshIndexArray));
  yyjson_val* mesh;
  std::size_t i;
  std::size_t max;
  yyjson_arr_foreach(meshIndexArray, i, max, mesh) {
    //construct mesh from filepath?
    meshGroup->meshIndices[i] = yyjson_get_uint(mesh);  // Get mesh index
    meshGroup->materialIndices[i] = yyjson_get_uint(yyjson_arr_get(materialIndexArray, i));  // Get material index
  }
  return meshGroup;
}

// MeshGroup::MeshGroup(GraphicsDevice* const device, const std::filesystem::path& path) : device(device) {
//   fastgltf::GltfFileStream fileStream(path);
//   loadData(fileStream);
// }
//
// MeshGroup::MeshGroup(GraphicsDevice* const device, yyjson_val* val) : device(device){
//   const char* bytes = yyjson_get_str(val);
//   const size_t len = yyjson_get_len(val);
//   auto dataBuffer = fastgltf::GltfDataBuffer::FromBytes(reinterpret_cast<const std::byte*>(bytes), len);
//   if (dataBuffer.error() != fastgltf::Error::None) GraphicsInstance::showError("failed to parse bytes in GLTF file");
//   loadData(dataBuffer.get());
// }

void MeshGroup::loadData(fastgltf::GltfDataGetter& dataGetter) {
  fastgltf::Asset asset;
  {
    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::EXT_meshopt_compression | fastgltf::Extensions::EXT_mesh_gpu_instancing | fastgltf::Extensions::KHR_accessor_float64);
    fastgltf::Expected<fastgltf::Asset> gltfAsset = parser.loadGltf(dataGetter, std::filesystem::canonical("../res"), fastgltf::Options::GenerateMeshIndices);
    if (gltfAsset.error() != fastgltf::Error::None) GraphicsInstance::showError("failed to parse GLTF file");
    asset = std::move(gltfAsset.get());
  }
  CommandBuffer commandBuffer;
  for (const fastgltf::Mesh& mesh: asset.meshes)
    for (const fastgltf::Primitive& primitive: mesh.primitives)
      meshes.push_back(std::make_shared<Mesh>(device, commandBuffer, asset, primitive));
  commandBuffer.preprocess();
  device->executeCommandBufferImmediate(commandBuffer);
}

const std::vector<std::shared_ptr<Mesh>>& MeshGroup::getMeshes() const { return meshes; }
