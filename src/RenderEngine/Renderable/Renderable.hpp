#pragma once

#include "Mesh.hpp"

#include <fastgltf/core.hpp>
#include <yyjson.h>


class Material;

class Renderable {
  GraphicsDevice* const device;
  std::vector<std::shared_ptr<Mesh>> meshes;

public:
  explicit Renderable(GraphicsDevice* device, const std::filesystem::path& path);
  explicit Renderable(GraphicsDevice* device, yyjson_val* val);

  void loadData(fastgltf::GltfDataGetter& dataGetter);
  [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& getMeshes() const;
};