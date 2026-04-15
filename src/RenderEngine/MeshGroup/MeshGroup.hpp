#pragma once

#include "Mesh.hpp"

#include "src/Component.hpp"

#include <yyjson.h>
#include <plf_colony.h>

class MeshGroup : public Component {
public:
  GraphicsDevice* const device;
  std::unordered_map<Mesh*, plf::colony<Mesh::InstanceID>> meshes;

  MeshGroup(std::uint64_t id, Entity& entity, GraphicsDevice* device, yyjson_val* val);
  ~MeshGroup() override;

  void onTick() override {}
};