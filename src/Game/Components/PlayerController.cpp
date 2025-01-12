#pragma once
#include "PlayerController.hpp"
#include <iostream>

#include "src/InputEngine/Input.hpp"

#include "Plant.hpp"
#include "src/Entity.hpp"
#include "src/Game/Game.hpp"

PlayerController::PlayerController(std::uint64_t id, Entity& entity) : Component(id, entity) {}

void PlayerController::onTick() {
  //move the player using keyboard
  if (Input::keyDown(Input::UP) > 0 || Input::keyDown(Input::W) > 0) {
    entity.position.y += movementSpeed;
  }
  if (Input::keyDown(Input::DOWN) > 0 || Input::keyDown(Input::S) > 0) {
    entity.position.y -= movementSpeed;
  }
  if (Input::keyDown(Input::LEFT) > 0 || Input::keyDown(Input::A) > 0) {
    entity.position.x -= movementSpeed;
  }
  if (Input::keyDown(Input::RIGHT) > 0 || Input::keyDown(Input::D) > 0) {
    entity.position.x += movementSpeed;
  }
  std::cout<<"Player position: "<<entity.position.x<<", "<<entity.position.y<<std::endl;
}

std::shared_ptr<Component> PlayerController::create(std::uint64_t id, Entity& entity, yyjson_val* obj) {
  return std::make_shared<PlayerController>(id, entity);
}
