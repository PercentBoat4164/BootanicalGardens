#include "Renderable.hpp"

Renderable::Renderable(const GraphicsDevice& device, const std::filesystem::path& path) : device(device) {
  fastgltf::GltfFileStream fileStream(path);
  loadData(fileStream);
}

Renderable::Renderable(const GraphicsDevice& device, yyjson_val* val) : device(device){
  const char* bytes = yyjson_get_str(val);
  size_t len = yyjson_get_len(val);
  auto dataBuffer = fastgltf::GltfDataBuffer::FromBytes(reinterpret_cast<const std::byte*>(bytes), len);
  if (dataBuffer.error() != fastgltf::Error::None) return; /**@todo Log this control path.*/
  loadData(dataBuffer.get());
}

void Renderable::loadData(fastgltf::GltfDataGetter& dataGetter) {
  {
    fastgltf::Parser parser(static_cast<fastgltf::Extensions>(0xffffff1));  // Enables all extensions because we do not know which extensions this file will require.
    fastgltf::Expected<fastgltf::Asset> gltfAsset = parser.loadGltf(dataGetter, std::filesystem::canonical("../res"), fastgltf::Options::GenerateMeshIndices);
    if (gltfAsset.error() != fastgltf::Error::None) return; /**@todo Log this control path.*/
    asset = std::move(gltfAsset.get());
  }
  for (const fastgltf::Mesh& mesh : asset.meshes)
    for (const fastgltf::Primitive& primitive : mesh.primitives)
      meshes.push_back(new Mesh(device, asset, primitive));  /**@todo Make a way to store Mesh data globally.*/
}

Renderable::~Renderable() {
  for (const Mesh* mesh : meshes) delete mesh;
}
