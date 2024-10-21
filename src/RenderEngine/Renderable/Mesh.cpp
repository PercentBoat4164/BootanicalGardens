#include "Mesh.hpp"

#include "Material.hpp"
#include "Vertex.hpp"
#include "src/Tools/StringHash.h"
#include "src/RenderEngine/Buffer.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <src/RenderEngine/CommandBuffer.hpp>

Mesh::Mesh(const GraphicsDevice& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Primitive& primitive) : material(primitive.materialIndex.has_value() ? new Material(device, commandBuffer, asset, asset.materials[primitive.materialIndex.value()]) : nullptr) {
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
  vertices.resize(asset.accessors[primitive.attributes[0].accessorIndex].count + 40);  /**@todo Why must we add 40? I have looked into this and I cannot answer that question.*/

  // Copy each attribute into the vector of vertices
  for (const fastgltf::Attribute& attribute : primitive.attributes) {
    auto& attributeAccessor = asset.accessors[attribute.accessorIndex];
    /**@todo Add support for other attributes (application specific, animations, more texture coordinates, more colors).*/
    switch (Tools::hash(attribute.name)) {  /**@see https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes */
      case Tools::hash("POSITION"):   fastgltf::copyFromAccessor<decltype(Vertex::position),            sizeof(Vertex)>(asset, attributeAccessor, vertices.data() + offsetof(Vertex, position));            break;
      case Tools::hash("NORMAL"):     fastgltf::copyFromAccessor<decltype(Vertex::normal),              sizeof(Vertex)>(asset, attributeAccessor, vertices.data() + offsetof(Vertex, normal));              break;
      case Tools::hash("TANGENT"):    fastgltf::copyFromAccessor<decltype(Vertex::tangent),             sizeof(Vertex)>(asset, attributeAccessor, vertices.data() + offsetof(Vertex, tangent));             break;
      case Tools::hash("TEXCOORD_0"): fastgltf::copyFromAccessor<decltype(Vertex::textureCoordinates0), sizeof(Vertex)>(asset, attributeAccessor, vertices.data() + offsetof(Vertex, textureCoordinates0)); break;
      case Tools::hash("COLOR_0"):    fastgltf::copyFromAccessor<decltype(Vertex::color0),              sizeof(Vertex)>(asset, attributeAccessor, vertices.data() + offsetof(Vertex, color0));              break;
      // case Tools::hash("JOINTS_0"): fastgltf::copyFromAccessor<decltype(Vertex::textureCoordinates0), sizeof(Vertex)>(asset, attributeAccessor, vertices.data() + offsetof(Vertex, textureCoordinates0)); break;
      // case Tools::hash("WEIGHTS_0"): fastgltf::copyFromAccessor<decltype(Vertex::textureCoordinates0), sizeof(Vertex)>(asset, attributeAccessor, vertices.data() + offsetof(Vertex, textureCoordinates0)); break;
      default: break;  /**@todo Log this unknown attribute.*/
    }
  }
  auto vertexBufferTemp = std::make_shared<Buffer>(device, "vertex upload buffer", vertices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
  vertexBuffer = std::make_shared<Buffer>(device, "vertex buffer", vertexBufferTemp->size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
  std::vector<VkBufferCopy> regions{{
    .srcOffset = 0,
    .dstOffset = 0,
    .size      = vertexBuffer->size() - 8
  }};
  commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(vertexBufferTemp, vertexBuffer, regions);

  // If this is an indexed mesh build the index buffer
  if (!primitive.indicesAccessor.has_value()) {
    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::copyFromAccessor<decltype(indices)::value_type>(asset, accessor, indices.data());
    auto indexBufferTemp = std::make_shared<Buffer>(device, "index upload buffer", indices, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    indexBuffer = std::make_shared<Buffer>(device, "index upload buffer", indexBufferTemp->size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    regions[0].size = indexBuffer->size() - 8;
    commandBuffer.record<CommandBuffer::CopyBufferToBuffer>(indexBufferTemp, indexBuffer, regions);
  }
}
