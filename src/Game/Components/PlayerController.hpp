#pragma once

#include "src/Component.hpp"

#include <utility>

class PlayerController : public Component {
public:
  explicit PlayerController(std::uint64_t id, const Entity& entity, simdjson::ondemand::object initializerObject);
  ~PlayerController() override = default;

  static std::unique_ptr<Component> create(std::uint64_t id, const Entity& entity, simdjson::ondemand::object i);

  void onTick() override;
};