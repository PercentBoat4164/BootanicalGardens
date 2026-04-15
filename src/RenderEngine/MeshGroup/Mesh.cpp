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

#include <cstddef>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>

#include <draco/core/decoder_buffer.h>

Mesh::Mesh(GraphicsDevice* device, yyjson_val* json) : device(device) {
  name = yyjson_get_str(yyjson_obj_get(json, "path"));
  const std::filesystem::path path = device->resourcesDirectory / "meshes" / name;
  fastgltf::GltfFileStream file(path);
  fastgltf::Asset asset;
  {  // Destroy the fastgltf::Parser and the fastgltf::Expected<> after they are no longer necessary
    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::EXT_meshopt_compression | fastgltf::Extensions::KHR_draco_mesh_compression);
    fastgltf::Expected<fastgltf::Asset> gltfAsset = parser.loadGltf(file, path.parent_path(), fastgltf::Options::GenerateMeshIndices | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DontRequireValidAssetMember);
    if (gltfAsset.error() != fastgltf::Error::None) GraphicsInstance::showError(std::string("failed to parse GLTF file '") + path.string() + "': " + std::string(magic_enum::enum_name(gltfAsset.error())));
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
  const std::size_t vertexCount = asset.accessors[primitive.attributes[0].accessorIndex].count;
  StagingBuffer positionsVertexBufferTemp(device, "Vertex Upload Buffer | Positions", vertexCount * sizeof(glm::vec3));
  StagingBuffer textureCoordinatesVertexBufferTemp(device, "Vertex Upload Buffer | Texture Coordinates", vertexCount * sizeof(glm::vec2));
  StagingBuffer normalsVertexBufferTemp(device, "Vertex Upload Buffer | Normals", vertexCount * sizeof(glm::vec3));
  StagingBuffer tangentsVertexBufferTemp(device, "Vertex Upload Buffer | Tangents", vertexCount * sizeof(glm::vec3));
  StagingBuffer indexBufferTemp(device, "Index Upload Buffer", asset.accessors[primitive.indicesAccessor.value()].count * sizeof(uint32_t));
  const std::shared_ptr<Buffer::BufferMapping> positionsVertexBufferTempMap = positionsVertexBufferTemp.map();
  const std::shared_ptr<Buffer::BufferMapping> textureCoordinatesVertexBufferTempMap = textureCoordinatesVertexBufferTemp.map();
  const std::shared_ptr<Buffer::BufferMapping> normalsVertexBufferTempMap = normalsVertexBufferTemp.map();
  const std::shared_ptr<Buffer::BufferMapping> tangentsVertexBufferTempMap = tangentsVertexBufferTemp.map();
  const std::shared_ptr<Buffer::BufferMapping> indexBufferTempMap = indexBufferTemp.map();
  if (primitive.dracoCompression) {
    std::size_t size;
    const char* byteBuf = std::visit(fastgltf::visitor {
      [&size](const auto&){
        return reinterpret_cast<const char*>(size = 0);
      },
      [&size](const fastgltf::sources::Array& array) -> const char* {
        size = array.bytes.size();
        return reinterpret_cast<const char*>(array.bytes.data());
      },
      [&size](const fastgltf::sources::Vector& vector) -> const char* {
        size = vector.bytes.size();
        return reinterpret_cast<const char*>(vector.bytes.data());
      }
    }, asset.buffers[asset.bufferViews[primitive.dracoCompression->bufferView].bufferIndex].data);
    draco::DecoderBuffer buffer;
    buffer.Init(byteBuf, size);
    draco::Decoder decoder;
    draco::StatusOr<std::unique_ptr<draco::Mesh>> const statusOrMesh = decoder.DecodeMeshFromBuffer(&buffer);
    if (!statusOrMesh.ok()) GraphicsInstance::showError("failed to decode Draco compressed mesh: '" + statusOrMesh.status().error_msg_string() + "', Status: '" + std::string(magic_enum::enum_name<draco::Status::Code>(statusOrMesh.status().code())) + "'");
    for (auto i = draco::FaceIndex(0); i < statusOrMesh.value()->num_faces(); ++i)
      std::memcpy(static_cast<uint32_t*>(indexBufferTempMap->data) + static_cast<size_t>(i.value() * 3), reinterpret_cast<const uint32_t*>(statusOrMesh.value()->face(i).data()), sizeof(uint32_t) * 3);
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

  positionsVertexBuffer = std::make_unique<Buffer>(device, (name + " | Positions").c_str(), positionsVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&positionsVertexBufferTemp, positionsVertexBuffer.get());

  textureCoordinatesVertexBuffer = std::make_unique<Buffer>(device, (name + " | Texture Coordinates").c_str(), textureCoordinatesVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&textureCoordinatesVertexBufferTemp, textureCoordinatesVertexBuffer.get());

  normalsVertexBuffer = std::make_unique<Buffer>(device, (name + " | Normals").c_str(), normalsVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&normalsVertexBufferTemp, normalsVertexBuffer.get());

  tangentsVertexBuffer = std::make_unique<Buffer>(device, (name + " | Tangents").c_str(), tangentsVertexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&tangentsVertexBufferTemp, tangentsVertexBuffer.get());

  indexBuffer = std::make_unique<Buffer>(device, (name + " | Indices").c_str(), indexBufferTemp.getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(&indexBufferTemp, indexBuffer.get());

  device->executeCommandBufferImmediate(commandBuffer);
}

void Mesh::InstanceGroup::reserveInstanceCount(const std::size_t count, CommandBuffer& commandBuffer) {
  if (reservedInstanceCount == count) return;

  Buffer* oldMaterialInstanceBuffer;
  Buffer* oldTransformInstanceBuffer;

  if (reservedInstanceCount != 0) {
    oldMaterialInstanceBuffer = materialInstanceBuffer.get();
    oldTransformInstanceBuffer = transformInstanceBuffer.get();

    commandBuffer.addCleanupResource(std::move(materialInstanceBuffer));
    commandBuffer.addCleanupResource(std::move(transformInstanceBuffer));
  }

  materialInstanceBuffer = std::make_unique<Buffer>(parent->device, (parent->name + " | Materials").c_str(), sizeof(Material::MaterialID) * count, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);
  transformInstanceBuffer = std::make_unique<Buffer>(parent->device, (parent->name + " | Transforms").c_str(), sizeof(glm::mat4) * count, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

  if (reservedInstanceCount != 0) {
    commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(oldMaterialInstanceBuffer, materialInstanceBuffer.get());
    commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(oldTransformInstanceBuffer, transformInstanceBuffer.get());
  }

  reservedInstanceCount = count;
}

Mesh::InstanceID Mesh::InstanceGroup::addInstance(const Material::MaterialID materialId, const glm::mat4& transform, CommandBuffer& commandBuffer) {
  // Allocate the temporary buffer for transfer
  auto tempBuffer = std::make_unique<Buffer>(parent->device, "Temporary transfer buffer", sizeof(Material::MaterialID) + sizeof(glm::mat4), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

  // Copy the data into the temporary buffer
  {
    const std::shared_ptr<Buffer::BufferMapping> tempBufferMap = tempBuffer->map();
    std::memcpy(tempBufferMap->data, &materialId, sizeof(Material::MaterialID));
    std::memcpy(static_cast<char*>(tempBufferMap->data) + sizeof(Material::MaterialID), &transform, sizeof(glm::mat4));
  }

  // Copy the data from the temporary buffer into the instance buffers
  VkBufferCopy copy {
    .srcOffset = 0,
    .dstOffset = instanceCount * sizeof(Material::MaterialID),
    .size = sizeof(Material::MaterialID)
  };
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(tempBuffer.get(), materialInstanceBuffer.get(), std::span{&copy, 1});
  copy = {
    .srcOffset = sizeof(Material::MaterialID),
    .dstOffset = instanceCount * sizeof(glm::mat4),
    .size = sizeof(glm::mat4)
  };
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(tempBuffer.get(), transformInstanceBuffer.get(), std::span{&copy, 1});
  commandBuffer.addCleanupResource(std::move(tempBuffer));

  // Record this instance
  const InstanceID instanceId = parent->instanceUuid;
  instanceIds.push_back(instanceId);
  // Update the parent mesh's instanceReferences
  parent->instanceReferences[instanceId] = {
    .material = material,
    .index = instanceCount
  };

  // Increment the instance counters
  ++instanceCount;
  return parent->instanceUuid++;
}

void Mesh::InstanceGroup::removeInstance(const size_t index, CommandBuffer& commandBuffer) {
  // Map the instance buffers, then copy the last instance data over the old instance data
  VkBufferCopy copy {
    .srcOffset = --instanceCount * sizeof(Material::MaterialID),
    .dstOffset = index * sizeof(Material::MaterialID),
    .size = sizeof(Material::MaterialID)
  };
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(materialInstanceBuffer.get(), materialInstanceBuffer.get(), std::span{&copy, 1});
  copy = {
    .srcOffset = instanceCount * sizeof(glm::mat4),
    .dstOffset = index * sizeof(glm::mat4),
    .size = sizeof(glm::mat4)
  };
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(transformInstanceBuffer.get(), transformInstanceBuffer.get(), std::span{&copy, 1});

  // Get the old instance's ID
  InstanceID& instanceId = instanceIds.at(index);
  // Update the parent mesh's instanceReferences
  parent->instanceReferences.erase(instanceId);
  // Copy the last instance's ID over the old instance's ID
  instanceId = instanceIds.at(instanceCount);
  instanceIds.pop_back();
}

Mesh::InstanceGroup& Mesh::getInstanceGroup(Material* const material) {
  const auto it = instanceGroups.find(material);
  if (it != instanceGroups.end()) return it->second;
  return instanceGroups.emplace(material, InstanceGroup{this, material, 1}).first->second;
}

Mesh::InstanceID Mesh::addInstance(Material* const material, const glm::mat4& transform, CommandBuffer& commandBuffer) {
  InstanceGroup& group = getInstanceGroup(material);
  if (group.instanceCount + 1 > group.reservedInstanceCount) {
    const std::size_t startingPoint = std::max(group.reservedInstanceCount, 1UL);
    group.reserveInstanceCount(startingPoint << 1, commandBuffer);
  }
  return group.addInstance(material->id, transform, commandBuffer);
}

void Mesh::removeInstance(const InstanceID id, CommandBuffer& commandBuffer) {
  InstanceReference& reference = instanceReferences.at(id);
  getInstanceGroup(reference.material).removeInstance(reference.index, commandBuffer);
  instanceReferences.erase(id);
}

void Mesh::updateInstance(const InstanceID id, Material* material, CommandBuffer& commandBuffer) {
  InstanceReference& reference = instanceReferences.at(id);
  InstanceGroup& oldGroup = instanceGroups.at(reference.material);
  InstanceGroup& newGroup = instanceGroups.try_emplace(material, this, material, 1).first->second;
  if (newGroup.instanceCount + 1 > newGroup.reservedInstanceCount)
    newGroup.reserveInstanceCount(newGroup.instanceCount * 2, commandBuffer);

  // Copy the instance from the old group into the new group
  VkBufferCopy copy {
    .srcOffset = reference.index * sizeof(Material::MaterialID),
    .dstOffset = newGroup.instanceCount * sizeof(Material::MaterialID),
    .size = sizeof(Material::MaterialID)
  };
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(oldGroup.materialInstanceBuffer.get(), newGroup.materialInstanceBuffer.get(), std::span{&copy, 1});
  copy = {
    .srcOffset = reference.index * sizeof(glm::mat4),
    .dstOffset = newGroup.instanceCount * sizeof(glm::mat4),
    .size = sizeof(glm::mat4)
  };
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(oldGroup.transformInstanceBuffer.get(), newGroup.transformInstanceBuffer.get(), std::span{&copy, 1});
  oldGroup.removeInstance(reference.index, commandBuffer);

  // Update the mesh's instance references
  reference.index = newGroup.instanceCount++;
  reference.material = material;
}

void Mesh::updateInstance(InstanceID id, const glm::mat4& transform, CommandBuffer& commandBuffer) {
  auto temp = std::make_unique<StagingBuffer>(device, std::format("Instance {} Transform Update Staging Buffer", id).c_str(), std::span{&transform, 1});

  const InstanceReference& reference = instanceReferences.at(id);
  const InstanceGroup& group = instanceGroups.at(reference.material);

  VkBufferCopy copy {
    .srcOffset = 0,
    .dstOffset = reference.index * sizeof(glm::mat4),
    .size = sizeof(glm::mat4)
  };
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(temp.get(), group.transformInstanceBuffer.get(), std::span{&copy, 1});
  commandBuffer.addCleanupResource(std::move(temp));
}

