#pragma once

#include "Mesh.hpp"

#include "src/Component.hpp"

#include <yyjson.h>
#include <plf_colony.h>

struct MeshGroup : Component {
  GraphicsDevice* const device;
  std::unordered_map<Mesh*, plf::colony<Mesh::InstanceReference>> meshes;

  MeshGroup(std::uint64_t id, Entity& entity, GraphicsDevice* device, yyjson_val* val);
  ~MeshGroup() override;

  void onTick() override;  // Move to an 'AnimationSystem'
};