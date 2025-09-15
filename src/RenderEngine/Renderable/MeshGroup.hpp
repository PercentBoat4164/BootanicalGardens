#pragma once

#include "src/Component.hpp"
#include "Mesh.hpp"

#include <fastgltf/core.hpp>

class yyjson_val;
class Material;

class MeshGroup : public Component {
  GraphicsDevice* const device = nullptr;
  glm::mat4 transformation;
  std::vector<std::uint64_t> meshIndices;
  std::vector<std::uint64_t> materialIndices;

public:
  MeshGroup(std::uint64_t id, Entity& entity);

  static std::shared_ptr<Component> create(std::uint64_t id, Entity& entity, yyjson_val* componentData);

  void loadData(fastgltf::GltfDataGetter& dataGetter);
  [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& getMeshes() const;
};