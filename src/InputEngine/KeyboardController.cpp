#include "KeyboardController.hpp"

#include "Input.hpp"

#include "../Entity.hpp"

KeyboardController::KeyboardController(std::string name) : Component(std::move(name)) {}

void KeyboardController::OnTick(Entity& entity) {
  if (Input::keyDown(Input::A) > 0) --entity.position.x;
  if (Input::keyDown(Input::SPACE) > 0) ++entity.position.z;
}
