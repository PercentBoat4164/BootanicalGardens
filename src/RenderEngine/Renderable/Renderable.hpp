#pragma once

#include "Mesh.hpp"

#include <fastgltf/core.hpp>
#include <yyjson.h>


class Material;
class Light;

class Renderable {
  const std::shared_ptr<GraphicsDevice> device;
  std::vector<std::shared_ptr<Mesh>> meshes;

public:
  explicit Renderable(const std::shared_ptr<GraphicsDevice>& device, const std::filesystem::path& path);
  explicit Renderable(const std::shared_ptr<GraphicsDevice>& device, yyjson_val* val);

  void loadData(fastgltf::GltfDataGetter& dataGetter);
  [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& getMeshes() const;
};