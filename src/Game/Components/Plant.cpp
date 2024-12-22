#include "Plant.hpp"

Plant::Plant(std::uint64_t id, const Entity& entity) : Component(id, entity) {}

void Plant::onTick() {
  // Checks collidable for enemy ghosts to eat
  // Eats ghosts?
}

std::unique_ptr<Component> Plant::create(std::uint64_t id, const Entity& entity, yyjson_val* obj) {
  return std::make_unique<Plant>(id, entity);
}
