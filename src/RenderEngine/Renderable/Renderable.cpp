#include "Renderable.hpp"

#include "src/RenderEngine/GraphicsInstance.hpp"

#include <src/RenderEngine/CommandBuffer.hpp>
#include <src/RenderEngine/GraphicsDevice.hpp>

Renderable::Renderable(GraphicsDevice* const device, const std::filesystem::path& path) : device(device) {
  fastgltf::GltfFileStream fileStream(path);
  loadData(fileStream);
}

Renderable::Renderable(GraphicsDevice* const device, yyjson_val* val) : device(device){
  const char* bytes = yyjson_get_str(val);
  const size_t len = yyjson_get_len(val);
  auto dataBuffer = fastgltf::GltfDataBuffer::FromBytes(reinterpret_cast<const std::byte*>(bytes), len);
  if (dataBuffer.error() != fastgltf::Error::None) GraphicsInstance::showError("failed to parse bytes in GLTF file");
  loadData(dataBuffer.get());
}

void Renderable::loadData(fastgltf::GltfDataGetter& dataGetter) {
  fastgltf::Asset asset;
  {
    fastgltf::Parser parser(fastgltf::Extensions::KHR_texture_transform | fastgltf::Extensions::KHR_texture_basisu | fastgltf::Extensions::MSFT_texture_dds | fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::EXT_meshopt_compression | fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::EXT_texture_webp | fastgltf::Extensions::KHR_materials_specular | fastgltf::Extensions::KHR_materials_ior | fastgltf::Extensions::KHR_materials_iridescence | fastgltf::Extensions::KHR_materials_volume | fastgltf::Extensions::KHR_materials_transmission | fastgltf::Extensions::KHR_materials_clearcoat | fastgltf::Extensions::KHR_materials_emissive_strength | fastgltf::Extensions::KHR_materials_sheen | fastgltf::Extensions::KHR_materials_unlit | fastgltf::Extensions::KHR_materials_anisotropy | fastgltf::Extensions::EXT_mesh_gpu_instancing | fastgltf::Extensions::MSFT_packing_normalRoughnessMetallic | fastgltf::Extensions::MSFT_packing_occlusionRoughnessMetallic | fastgltf::Extensions::KHR_materials_dispersion | fastgltf::Extensions::KHR_materials_variants | fastgltf::Extensions::KHR_accessor_float64);
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

const std::vector<std::shared_ptr<Mesh>>& Renderable::getMeshes() const { return meshes; }
