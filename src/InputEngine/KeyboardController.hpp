#pragma once

#include "src/Component.hpp"

#include <utility>

class KeyboardController : public Component {
public:
  explicit KeyboardController(std::string name);
  ~KeyboardController() override = default;

  void OnTick(Entity& entity) override;
};