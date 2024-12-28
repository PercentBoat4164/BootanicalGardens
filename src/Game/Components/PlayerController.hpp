#pragma once

#include "src/Component.hpp"

#include <utility>

struct yyjson_val;

/**
 * Allows the player to control an entity using keyboard input.
 */
class PlayerController : public Component {
public:
  explicit PlayerController(std::uint64_t id, const Entity& entity);
  ~PlayerController() override = default;

  static std::shared_ptr<Component> create(std::uint64_t id, const Entity& entity, yyjson_val* obj);

  void onTick() override;
};