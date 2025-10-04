#pragma once

#include <string>
#include <utility>
#include <memory>
#include "RenderEngine/Renderable/Mesh.hpp"

class Entity;

class Component {
protected:
  std::uint64_t id;
  Entity& entity;

public:
  Component(std::uint64_t id, Entity& entity);
  Component(const Component& other)      = default;
  Component(Component&& other) noexcept  = default;
  Component& operator=(const Component&) = delete;
  Component& operator=(Component&&)      = delete;
  virtual ~Component()                   = 0;

  virtual void onTick() = 0;

  [[nodiscard]] std::uint64_t getId() const;
};