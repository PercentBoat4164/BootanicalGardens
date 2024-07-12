#include "Plant.hpp"

Plant::Plant(std::string name, std::shared_ptr<Entity>& entity) : Component(std::move(name), entity) {}

void Plant::onTick() {
  // Checks collidable for enemy ghosts to eat
  //   Eats ghosts?
}
