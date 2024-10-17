#include "PlayerController.hpp"

#include "src/InputEngine/Input.hpp"

#include "Plant.hpp"
#include "src/Entity.hpp"
#include "src/Game/Game.hpp"

PlayerController::PlayerController(std::uint64_t id, const Entity& entity, yyjson_val* initializerObject) : Component(id, entity) {}

void PlayerController::onTick() {
  if (Input::keyDown(Input::P) > 0) {
    Entity& myPlant = Game::addEntity(entity);
    myPlant.addComponent<Plant>();
  }
}
std::unique_ptr<Component> PlayerController::create(std::uint64_t id, const Entity& entity, yyjson_val* i) {
  return std::make_unique<PlayerController>(id, entity, i);
}
