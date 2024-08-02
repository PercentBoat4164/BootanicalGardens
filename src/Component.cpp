#include "Component.hpp"

Component::~Component() = default;

std::uint64_t Component::getId() const {
  return id;
}
