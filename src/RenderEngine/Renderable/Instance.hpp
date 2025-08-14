#pragma once

#include <glm/matrix.hpp>

#include <memory>

class Mesh;
struct Instance {
  std::shared_ptr<Mesh> mesh;
  glm::mat4 modelToWorld{};
  uint32_t materialID;
};