#include "Plant.hpp"

Plant::Plant(std::uint64_t id, Entity& entity) : Component(id, entity) {}

void Plant::onTick() {
  // Checks collidable for enemy ghosts to eat
  // Eats ghosts?
}

std::shared_ptr<Component> Plant::create(std::uint64_t id, Entity& entity, yyjson_val* obj) {
  return std::make_shared<Plant>(id, entity);
}
