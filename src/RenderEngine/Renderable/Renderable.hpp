#pragma once

#include "Mesh.hpp"

#include <fastgltf/core.hpp>
#include <yyjson.h>


class Material;
class Light;

class Renderable {
  const GraphicsDevice& device;
  fastgltf::Asset asset;
  std::vector<Mesh*> meshes;

public:
  explicit Renderable(const GraphicsDevice& device, const std::filesystem::path& path);
  explicit Renderable(const GraphicsDevice& device, yyjson_val* val);

  void loadData(fastgltf::GltfDataGetter& dataGetter);

  ~Renderable();
};