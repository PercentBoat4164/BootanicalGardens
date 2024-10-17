#include "Mesh.hpp"

#include "Material.hpp"
#include "src/Tools/StringHash.h"

Mesh::Mesh(const GraphicsDevice& device, const fastgltf::Asset& asset, const fastgltf::Primitive& primitive) :
    material(primitive.materialIndex.has_value() ? new Material(device, asset, asset.materials[primitive.materialIndex.value()]) : nullptr) {
  if (primitive.type != fastgltf::PrimitiveType::Triangles) return; /**@todo Cannot handle this situation yet.*/
  if (primitive.indicesAccessor.has_value()) {
    auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
    indices.resize(accessor.count);
    fastgltf::copyFromAccessor<decltype(indices)::value_type>(asset, accessor, indices.data());
    for (const fastgltf::Attribute& attribute : primitive.attributes) {
      auto& attributeAccessor = asset.accessors[attribute.accessorIndex];
      switch (Tools::hash(attribute.name)) {
        case Tools::hash("POSITION"):
          positions.resize(attributeAccessor.count);
          fastgltf::copyFromAccessor<decltype(positions)::value_type>(asset, attributeAccessor, positions.data());
          break;
        case Tools::hash("NORMAL"):
          normals.resize(attributeAccessor.count);
          fastgltf::copyFromAccessor<decltype(normals)::value_type>(asset, attributeAccessor, normals.data());
          break;
        case Tools::hash("TANGENT"):
          tangents.resize(attributeAccessor.count);
          fastgltf::copyFromAccessor<decltype(tangents)::value_type>(asset, attributeAccessor, tangents.data());
          break;
        case Tools::hash("TEXCOORD_0"):
          textureCoordinates.resize(attributeAccessor.count);
          fastgltf::copyFromAccessor<decltype(textureCoordinates)::value_type>(asset, attributeAccessor, textureCoordinates.data());
          break;
        default: break;  /**@todo Log this unknown attribute.*/
      }
    }
  }
}

Mesh::~Mesh() {
  delete material;
  material = nullptr;
}
