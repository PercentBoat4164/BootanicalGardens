#include "src/RenderEngine/MeshGroup/Mesh.hpp"

#include "draco/compression/decode.h"
#include "draco/mesh/mesh.h"
#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/Resources/StagingBuffer.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/MeshGroup/Vertex.hpp"
#include "src/Tools/Hashing.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>

#include <draco/core/decoder_buffer.h>

Mesh::Mesh(GraphicsDevice* device, yyjson_val* json) : device(device) {
  std::filesystem::path path = device->resourcesDirectory / "meshes" / yyjson_get_str(yyjson_obj_get(json, "path"));
  fastgltf::GltfFileStream file(path);
  fastgltf::Asset asset;
  {  // Destroy the fastgltf::Parser and the fastgltf::Expected<> after they are no longer necessary
    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::EXT_meshopt_compression | fastgltf::Extensions::KHR_draco_mesh_compression);
    fastgltf::Expected<fastgltf::Asset> gltfAsset = parser.loadGltf(file, path.parent_path(), fastgltf::Options::GenerateMeshIndices | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DontRequireValidAssetMember);
    if (gltfAsset.error() != fastgltf::Error::None) GraphicsInstance::showError(std::string("failed to parse GLTF file '") + path.string() + "': " + magic_enum::enum_name(gltfAsset.error()));
    asset = std::move(gltfAsset.get());
  }
  CommandBuffer commandBuffer;
  if (asset.meshes.size() != 1) GraphicsInstance::showError("This error should really be a warning. Only the first mesh in the asset '" + path.string() + "' will be imported. All others will be ignored.");
  if (asset.meshes[0].primitives.size() != 1) GraphicsInstance::showError("This error should really be a warning. Only the first primitive in the first mesh in the asset '" + path.string() + "' will be imported. All others will be ignored.");
  fastgltf::Primitive& primitive = asset.meshes[0].primitives[0];

  // Determine this mesh's topology
  switch (primitive.type) {
    case fastgltf::PrimitiveType::Points: topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
    case fastgltf::PrimitiveType::Lines: topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
    case fastgltf::PrimitiveType::LineLoop:
    case fastgltf::PrimitiveType::LineStrip: topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
    case fastgltf::PrimitiveType::Triangles: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
    case fastgltf::PrimitiveType::TriangleStrip: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
    case fastgltf::PrimitiveType::TriangleFan: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
  }

  // Determine vertex count (GLTF spec states that "All attribute accessors for a given primitive <b>MUST</b> have the same <c>count</c>." Therefore, the count of the first accessor for this primitive is used to decide the vertex count)
  std::size_t vertexCount = asset.accessors[primitive.attributes[0].accessorIndex].count;
  StagingBuffer positionsVertexBufferTemp(device, "Vertex Upload Buffer | Positions", vertexCount * sizeof(glm::vec3));
  StagingBuffer textureCoordinatesVertexBufferTemp(device, "Vertex Upload Buffer | Texture Coordinates", vertexCount * sizeof(glm::vec2));
  StagingBuffer normalsVertexBufferTemp(device, "Vertex Upload Buffer | Normals", vertexCount * sizeof(glm::vec3));
  StagingBuffer tangentsVertexBufferTemp(device, "Vertex Upload Buffer | Tangents", vertexCount * sizeof(glm::vec3));
  StagingBuffer indexBufferTemp(device, "Index Upload Buffer", asset.accessors[primitive.indicesAccessor.value()].count * sizeof(uint32_t));
  std::shared_ptr<Buffer::BufferMapping> positionsVertexBufferTempMap = positionsVertexBufferTemp.map();
  std::shared_ptr<Buffer::BufferMapping> textureCoordinatesVertexBufferTempMap = textureCoordinatesVertexBufferTemp.map();
  std::shared_ptr<Buffer::BufferMapping> normalsVertexBufferTempMap = normalsVertexBufferTemp.map();
  std::shared_ptr<Buffer::BufferMapping> tangentsVertexBufferTempMap = tangentsVertexBufferTemp.map();
  std::shared_ptr<Buffer::BufferMapping> indexBufferTempMap = indexBufferTemp.map();
  if (primitive.dracoCompression) {
    std::size_t size;
    const char* byteBuf = std::visit(fastgltf::visitor {
      [&size](const auto& arg){
        return reinterpret_cast<const char*>(size = 0);
      },
      [&size](const fastgltf::sources::Array& array){
        size = array.bytes.size();
        return reinterpret_cast<const char*>(array.bytes.data());
      },
      [&size](const fastgltf::sources::Vector& vector){
        size = vector.bytes.size();
        return reinterpret_cast<const char*>(vector.bytes.data());
      }
    }, asset.buffers[asset.bufferViews[primitive.dracoCompression->bufferView].bufferIndex].data);
    draco::DecoderBuffer buffer;
    buffer.Init(byteBuf, size);
    draco::Decoder decoder;
    draco::StatusOr<std::unique_ptr<draco::Mesh>> statusOrMesh = decoder.DecodeMeshFromBuffer(&buffer);
    if (!statusOrMesh.ok()) GraphicsInstance::showError("failed to decode Draco compressed mesh: '" + statusOrMesh.status().error_msg_string() + "', Status: '" + magic_enum::enum_name<draco::Status::Code>(statusOrMesh.status().code()) + "'");
    for (auto i = draco::FaceIndex(0); i < statusOrMesh.value()->num_faces(); ++i)
      std::memcpy(static_cast<uint32_t*>(indexBufferTempMap->data) + i.value() * 3, reinterpret_cast<const uint32_t*>(statusOrMesh.value()->face(i).data()), sizeof(uint32_t) * 3);
    const draco::PointAttribute* position = statusOrMesh.value()->GetNamedAttribute(draco::GeometryAttribute::POSITION);
    const draco::PointAttribute* texCoords = statusOrMesh.value()->GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);
    const draco::PointAttribute* normals = statusOrMesh.value()->GetNamedAttribute(draco::GeometryAttribute::NORMAL);
    const draco::PointAttribute* tangents = statusOrMesh.value()->GetNamedAttribute(draco::GeometryAttribute::GENERIC);  /**@todo: Find out how to get the tangents out of the draco mesh.*/
    /**@todo: Ensure that the format of the draco data matches that of the Vertex. attribute->data_type && attribute->num_components*/
    // std::memcpy(positionsVertexBufferTempMap->data, position->buffer()->data(), positionsVertexBufferTempMap->buffer->getSize());
    if (statusOrMesh.value()->num_points() != vertexCount) GraphicsInstance::showError("Could not decode the expected number of vertices!");
    for (auto i = draco::PointIndex(0); i < statusOrMesh.value()->num_points(); ++i) {
      position->ConvertValue<float, 3>(position->mapped_index(i), reinterpret_cast<float*>(static_cast<glm::vec3*>(positionsVertexBufferTempMap->data) + i.value()));
      texCoords->ConvertValue<float, 2>(texCoords->mapped_index(i), reinterpret_cast<float*>(static_cast<glm::vec2*>(textureCoordinatesVertexBufferTempMap->data) + i.value()));
      normals->ConvertValue<float, 3>(normals->mapped_index(i), reinterpret_cast<float*>(static_cast<glm::vec3*>(normalsVertexBufferTempMap->data) + i.value()));
      tangents->ConvertValue<float, 3>(tangents->mapped_index(i), reinterpret_cast<float*>(static_cast<glm::vec3*>(tangentsVertexBufferTempMap->data) + i.value()));
    }
  } else {
    fastgltf::copyFromAccessor<uint32_t>(asset, asset.accessors[primitive.indicesAccessor.value()], indexBufferTempMap->data);
    if (auto* attribute = primitive.findAttribute("POSITION")) fastgltf::copyFromAccessor<glm::vec3, sizeof(glm::vec3)>(asset, asset.accessors[attribute->accessorIndex], positionsVertexBufferTempMap->data);
    else std::memset(positionsVertexBufferTempMap->data, 0, positionsVertexBufferTempMap->buffer->getSize());
    if (auto* attribute = primitive.findAttribute("TEXCOORD_0")) fastgltf::copyFromAccessor<glm::vec2, sizeof(glm::vec2)>(asset, asset.accessors[attribute->accessorIndex], textureCoordinatesVertexBufferTempMap->data);
    else std::memset(textureCoordinatesVertexBufferTempMap->data, 0, textureCoordinatesVertexBufferTempMap->buffer->getSize());
    if (auto* attribute = primitive.findAttribute("NORMAL")) fastgltf::copyFromAccessor<glm::vec3, sizeof(glm::vec3)>(asset, asset.accessors[attribute->accessorIndex], normalsVertexBufferTempMap->data);
    else std::memset(normalsVertexBufferTempMap->data, 0, normalsVertexBufferTempMap->buffer->getSize());
    if (auto* attribute = primitive.findAttribute("TANGENT")) fastgltf::copyFromAccessor<glm::vec3, sizeof(glm::vec3)>(asset, asset.accessors[attribute->accessorIndex], tangentsVertexBufferTempMap->data);
    else std::memset(tangentsVertexBufferTempMap->data, 0, tangentsVertexBufferTempMap->buffer->getSize());
  }

  positionsVertexBuffer = std::make_unique<Buffer>(device, "Vertex Buffer | Positions", positionsVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&positionsVertexBufferTemp, positionsVertexBuffer.get());

  textureCoordinatesVertexBuffer = std::make_unique<Buffer>(device, "Vertex Buffer | Texture Coordinates", textureCoordinatesVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&textureCoordinatesVertexBufferTemp, textureCoordinatesVertexBuffer.get());

  normalsVertexBuffer = std::make_unique<Buffer>(device, "Vertex Buffer | Normals", normalsVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&normalsVertexBufferTemp, normalsVertexBuffer.get());

  tangentsVertexBuffer = std::make_unique<Buffer>(device, "Vertex Buffer | Tangents", tangentsVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&tangentsVertexBufferTemp, tangentsVertexBuffer.get());

  indexBuffer = std::make_unique<Buffer>(device, "Index Buffer", indexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&indexBufferTemp, indexBuffer.get());

  commandBuffer.preprocess();
  device->executeCommandBufferImmediate(commandBuffer);
}

Mesh::InstanceReference Mesh::addInstance(uint64_t materialID, glm::mat4 mat) {
  Material* material = indexBuffer->device->getMaterial(materialID);
  InstanceCollection& instanceCollection = instances[material];
  InstanceReference instanceReference;
  instanceReference.material = material;
  instanceReference.modelInstanceID = instanceCollection.modelInstances.emplace(mat);
  instanceReference.materialInstanceID = instanceCollection.materialInstances.emplace(materialID);
  instanceReference.perInstanceDataID = instanceCollection.perInstanceData.emplace(mat);
  instanceCollection.stale = true;
  stale = true;
  return instanceReference;
}

void Mesh::removeInstance(InstanceReference&& instanceReference) {
  InstanceCollection& instanceCollection = instances[instanceReference.material];
  instanceCollection.modelInstances.erase(instanceReference.modelInstanceID);
  instanceCollection.materialInstances.erase(instanceReference.materialInstanceID);
  instanceCollection.perInstanceData.erase(instanceReference.perInstanceDataID);
  instanceCollection.stale = true;
  stale = true;
}

void Mesh::update(CommandBuffer& commandBuffer) {
  if (!stale) return;
  for (InstanceCollection& instanceCollection: instances | std::ranges::views::values) {
    if (!instanceCollection.stale) continue;
    if (instanceCollection.perInstanceData.empty()) {
      instanceCollection.materialInstanceBuffer = nullptr;
      instanceCollection.modelInstanceBuffer = nullptr;
    } else {
      if (const std::size_t materialSize = instanceCollection.materialInstances.size() * sizeof(decltype(InstanceCollection::materialInstances)::value_type); instanceCollection.materialInstanceBuffer == nullptr || instanceCollection.materialInstanceBuffer->getSize() != materialSize) {
        // Re-build the buffers
        const std::size_t modelSize = instanceCollection.modelInstances.size() * sizeof(decltype(InstanceCollection::modelInstances)::value_type);
        instanceCollection.materialInstanceBuffer = nullptr;  // Make sure that the old buffer has been deleted before building the new one.
        instanceCollection.modelInstanceBuffer = nullptr;
        instanceCollection.materialInstanceBuffer = std::make_unique<Buffer>(device, "Mesh-Material Instance Buffer", materialSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
        instanceCollection.modelInstanceBuffer = std::make_unique<Buffer>(device, "Mesh-Transform Instance Buffer", modelSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
      } {  // Update the material buffer
        auto* stagingBuffer = new StagingBuffer(device, "Mesh-Material Instance Staging Buffer", instanceCollection.materialInstanceBuffer->getSize());
        const std::shared_ptr<Buffer::BufferMapping> map = stagingBuffer->map();
        std::size_t i = std::numeric_limits<std::size_t>::max();
        for (const MaterialInstance& materialInstance: instanceCollection.materialInstances)
          static_cast<MaterialInstance*>(map->data)[++i] = materialInstance;
        commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(stagingBuffer, instanceCollection.materialInstanceBuffer.get());
        commandBuffer.addCleanupResource(stagingBuffer);
      } {  // Update the model buffer
        auto* stagingBuffer = new StagingBuffer(device, "Mesh-Model Instance Staging Buffer", instanceCollection.modelInstanceBuffer->getSize());
        const std::shared_ptr<Buffer::BufferMapping> map = stagingBuffer->map();
        std::size_t i = std::numeric_limits<std::size_t>::max();
        for (const ModelInstance& modelInstance: instanceCollection.modelInstances)
          static_cast<ModelInstance*>(map->data)[++i] = modelInstance;
        commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(stagingBuffer, instanceCollection.modelInstanceBuffer.get());
        commandBuffer.addCleanupResource(stagingBuffer);
      }
    }
    instanceCollection.stale = false;
  }
  stale = false;
}