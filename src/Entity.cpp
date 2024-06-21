#include "Entity.hpp"

void Entity::removeComponent(const std::string& name) {
  components.erase(name);
}
