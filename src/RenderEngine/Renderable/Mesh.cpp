#include "src/RenderEngine/Renderable/Mesh.hpp"

#include "../Resources/Buffer.hpp"
#include "../Resources/StagingBuffer.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Texture.hpp"
#include "src/RenderEngine/Renderable/Vertex.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"
#include "src/Tools/StringHash.h"

#include <fastgltf/glm_element_traits.hpp>
#include <volk/volk.h>

Mesh::Mesh(const std::shared_ptr<GraphicsDevice>& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Primitive& primitive) :
material(primitive.materialIndex.has_value() ? std::make_shared<Material>(device, commandBuffer, asset, asset.materials[primitive.materialIndex.value()]) : std::make_shared<Material>()) {
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
  vertices.resize(asset.accessors[primitive.attributes[0].accessorIndex].count);

  // Copy each attribute into the vector of vertices
  for (const fastgltf::Attribute& attribute: primitive.attributes) {
    /**@todo: Add support for other attributes (application specific, animations, more texture coordinates, more colors).*/
    switch (Tools::hash(attribute.name)) { /**@see https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes */
      case Tools::hash("POSITION"): fastgltf::copyFromAccessor<decltype(Vertex::position), sizeof(Vertex)>(asset, asset.accessors[attribute.accessorIndex], reinterpret_cast<char*>(vertices.data()) + offsetof(Vertex, position)); break;
      case Tools::hash("NORMAL"): fastgltf::copyFromAccessor<decltype(Vertex::normal), sizeof(Vertex)>(asset, asset.accessors[attribute.accessorIndex], reinterpret_cast<char*>(vertices.data()) + offsetof(Vertex, normal)); break;
      case Tools::hash("TANGENT"): fastgltf::copyFromAccessor<decltype(Vertex::tangent), sizeof(Vertex)>(asset, asset.accessors[attribute.accessorIndex], reinterpret_cast<char*>(vertices.data()) + offsetof(Vertex, tangent)); break;
      case Tools::hash("TEXCOORD_0"): fastgltf::copyFromAccessor<decltype(Vertex::textureCoordinates0), sizeof(Vertex)>(asset, asset.accessors[attribute.accessorIndex], reinterpret_cast<char*>(vertices.data()) + offsetof(Vertex, textureCoordinates0)); break;
      case Tools::hash("COLOR_0"): fastgltf::copyFromAccessor<decltype(Vertex::color0), sizeof(Vertex)>(asset, asset.accessors[attribute.accessorIndex], reinterpret_cast<char*>(vertices.data()) + offsetof(Vertex, color0)); break;
      default: GraphicsInstance::showError(std::string("unknown vertex attribute name: " + attribute.name)); break;
    }
  }

  // for (Vertex& vertex : vertices) vertex.position.z *= -1;

  auto vertexBufferTemp = std::make_shared<StagingBuffer>(device, "vertex upload buffer", vertices.data(), vertices.size());
  vertexBuffer          = std::make_shared<Buffer>(device, "vertex buffer", vertexBufferTemp->getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
  std::vector<VkBufferCopy> regions{{.srcOffset = 0, .dstOffset = 0, .size = vertexBuffer->getSize()}};
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(vertexBufferTemp, vertexBuffer, regions);

  // If this is an indexed mesh build the index buffer
  if (primitive.indicesAccessor.has_value()) {
    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::copyFromAccessor<decltype(indices)::value_type>(asset, accessor, indices.data());
    auto indexBufferTemp = std::make_shared<StagingBuffer>(device, "index upload buffer", indices);
    indexBuffer          = std::make_shared<Buffer>(device, "index buffer", indexBufferTemp->getSize(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
    regions[0].size = indexBuffer->getSize();
    commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(indexBufferTemp, indexBuffer, regions);
  }
}

void Mesh::update(const RenderGraph& graph) const {
  // uniformBuffer->update(glm::identity<glm::mat4>());
}

bool Mesh::isOpaque() const { return material->getAlphaMode() != fastgltf::AlphaMode::Blend; }
bool Mesh::isTransparent() const { return material->getAlphaMode() == fastgltf::AlphaMode::Blend; }
std::shared_ptr<Material> Mesh::getMaterial() const { return material; }
std::shared_ptr<Buffer> Mesh::getVertexBuffer() const { return vertexBuffer; }
std::shared_ptr<Buffer> Mesh::getIndexBuffer() const { return indexBuffer; }
