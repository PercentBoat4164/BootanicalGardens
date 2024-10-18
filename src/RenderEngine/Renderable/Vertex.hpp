#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>

#include "fastgltf/tools.hpp"

class Vertex {
public:
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec4 tangent{};
  glm::vec2 textureCoordinates0{};
  glm::vec3 color0{};

  bool operator==(const Vertex& other) const;
};

template<>
struct fastgltf::ElementTraits<Vertex> : ElementTraitsBase<decltype(Vertex::position), AccessorType::Vec3, float> {};