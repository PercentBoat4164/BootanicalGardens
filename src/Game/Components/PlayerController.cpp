#include "PlayerController.hpp"

#include "src/InputEngine/Input.hpp"

#include "Plant.hpp"
#include "src/Entity.hpp"
#include "src/Game/Game.hpp"

#include <iostream>

PlayerController::PlayerController(const std::uint64_t id, Entity& entity) : Component(id, entity) {}

void PlayerController::onTick() {
  //move the player using keyboard
  if (Input::keyDown(SDLK_UP) > 0 || Input::keyDown(SDLK_W) > 0) {
    entity.position.y += movementSpeed;
  }
  if (Input::keyDown(SDLK_DOWN) > 0 || Input::keyDown(SDLK_S) > 0) {
    entity.position.y -= movementSpeed;
  }
  if (Input::keyDown(SDLK_LEFT) > 0 || Input::keyDown(SDLK_A) > 0) {
    entity.position.x -= movementSpeed;
  }
  if (Input::keyDown(SDLK_RIGHT) > 0 || Input::keyDown(SDLK_D) > 0) {
    entity.position.x += movementSpeed;
  }
  std::cout<<"Player position: "<<entity.position.x<<", "<<entity.position.y<<std::endl;
}

std::shared_ptr<Component> PlayerController::create(std::uint64_t id, Entity& entity, yyjson_val* obj) {
  return std::make_shared<PlayerController>(id, entity);
}
