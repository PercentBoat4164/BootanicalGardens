#include "PlayerController.hpp"

#include "src/InputEngine/Input.hpp"

#include "Plant.hpp"
#include "src/Entity.hpp"
#include "src/Game/Game.hpp"

#include <iostream>
#include <functional>

class Iput {
public:  // Way 1
    struct OnKeyDown {
        virtual void onKeyDown(int key) = 0;
    };
    struct OnKeyUp {
        virtual void onKeyUp(int key) = 0;
    };

public:  // Way 2
    static std::vector<std::function<void(int key)>> onKeyDowns;

    static void registerKeyDown(std::function<void(int key)> oKD) {
        onKeyDowns.push_back(oKD);
    }
};

// Inheritance is only required for way 1
class PCont : public Iput::OnKeyDown, public Iput::OnKeyUp {
public: // Way 2
    PCont() {
        Iput::registerKeyDown([this](auto key){onKeyDown(key);});
    }

public: // Way 1
    void onKeyDown(int key) override {};
    void onKeyUp(int key) override {};
};

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
}

std::shared_ptr<Component> PlayerController::create(std::uint64_t id, Entity& entity, yyjson_val* obj) {
  return std::make_shared<PlayerController>(id, entity);
}
