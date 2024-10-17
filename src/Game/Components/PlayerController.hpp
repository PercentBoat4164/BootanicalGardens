#pragma once

#include "src/Component.hpp"

#include <utility>

struct yyjson_val;

class PlayerController : public Component {
public:
  explicit PlayerController(std::uint64_t id, const Entity& entity, yyjson_val* initializerObject);
  ~PlayerController() override = default;

  static std::unique_ptr<Component> create(std::uint64_t id, const Entity& entity, yyjson_val* i);

  void onTick() override;
};