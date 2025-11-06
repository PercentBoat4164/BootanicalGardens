#include "Entity.hpp"

#include "src/Game/Components/PlayerController.hpp"
#include "src/Game/Components/Plant.hpp"

#include <yyjson.h>

std::unordered_map<std::string, Entity::ComponentConstructor> Entity::componentConstructors {
  {"PlayerController", &PlayerController::create},
  {"Plant", &Plant::create},
};

bool Entity::registerComponentConstructor(const std::string& name, const ComponentConstructor& function) {
  if (componentConstructors.contains(name)) return false;
  componentConstructors.insert({name, function});
  return true;
}

Entity::Entity(const std::uint64_t id, const glm::vec3 position, const glm::quat& rotation, const glm::vec3 scale)
    : id(id), position(position), rotation(rotation), scale(scale) {}

Entity::Entity(const std::uint64_t id, const Entity& other)
    : id(id), position(other.position), rotation(other.rotation), scale(other.scale) {}

Component* Entity::addComponent(yyjson_val* componentData) {
  auto it = componentConstructors.find(yyjson_get_str(yyjson_obj_get(componentData, "type")));
  if (it == componentConstructors.end()) return nullptr;
  std::shared_ptr<Component> component = (it->second)(++nextComponentId, *this, componentData);
  return components.emplace(component->getId(), std::move(component)).first->second.get();
}

void Entity::removeComponent(const uint64_t id) {
  components.erase(id);
}
