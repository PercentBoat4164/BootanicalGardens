#pragma once

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/core.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <vector>

class GraphicsDevice;
class Material;

class Mesh {
  std::vector<uint32_t> indices;
  std::vector<glm::vec3> positions;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec4> tangents;
  std::vector<glm::vec2> textureCoordinates;
  Material* material{nullptr};

public:
  Mesh(const GraphicsDevice& device, const fastgltf::Asset& asset, const fastgltf::Primitive& primitive);
  ~Mesh();
};
