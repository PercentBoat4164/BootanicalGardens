#pragma once

#include "Mesh.hpp"

#include "src/Component.hpp"

#include <yyjson.h>
#include <vector>

struct MeshGroup {
  GraphicsDevice* device;
  std::unordered_map<Mesh*, std::vector<Mesh::InstanceReference>> meshes;

  MeshGroup(GraphicsDevice* device, yyjson_val* val) noexcept;
  MeshGroup(MeshGroup&& other) noexcept : device(other.device), meshes(std::move(other.meshes)) {}

  MeshGroup& operator=(MeshGroup&& other) noexcept {
    device = other.device;
    meshes = std::move(other.meshes);
    return *this;
  }
};

#include "src/EntityComponentSystem/AoSColumn.hpp"

using MeshGroupColumn = AoSColumn<1514, MeshGroup>;

void foo(GraphicsDevice* device, const std::vector<yyjson_val*>& val);