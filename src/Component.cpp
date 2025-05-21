#include "Component.hpp"

#include "Entity.hpp"

Component::Component(std::uint64_t id, Entity& entity) : id(id), entity(entity) {}

Component::~Component() = default;

std::uint64_t Component::getId() const {
  return id;
}
