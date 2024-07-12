#pragma once

#include "src/Component.hpp"

#include <utility>

class PlayerController : public Component {
public:
  PlayerController(std::string name, std::shared_ptr<Entity>& entity);
  ~PlayerController() override = default;

  void onTick() override;
};