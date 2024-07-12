#include "PlayerController.hpp"

#include "src/InputEngine/Input.hpp"

#include "Plant.hpp"
#include "src/Entity.hpp"
#include "src/Game/Game.hpp"

PlayerController::PlayerController(std::string name, std::shared_ptr<Entity>& entity) : Component(std::move(name), entity) {}

void PlayerController::onTick() {
  if (Input::keyDown(Input::A) > 0) --entity->position.x;
  if (Input::keyDown(Input::SPACE) > 0) ++entity->position.z;

  if (Input::keyDown(Input::P) > 0) {
    std::shared_ptr<Entity> myPlant = Game::addEntity();
    myPlant->addComponent(std::make_shared<Plant>("myPlant", myPlant));
    myPlant->position = entity->position;
  }
}