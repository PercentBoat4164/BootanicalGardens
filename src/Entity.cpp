#include "Entity.hpp"

#include "src/Game/Components/PlayerController.hpp"
#include "src/Game/Components/Plant.hpp"

#include <yyjson.h>

std::unordered_map<std::string, void*> Entity::componentConstructors {
  {"PlayerController", reinterpret_cast<void*>(&PlayerController::create)},
  {"Plant", reinterpret_cast<void*>(&Plant::create)},
};

bool Entity::registerComponentConstructor(const std::string& name, void* function) {
  if (componentConstructors.find(name) != componentConstructors.end()) return false;
  componentConstructors.insert({name, function});
  return true;
}

Entity::Entity(std::uint64_t id, glm::vec3 position, glm::dquat rotation, glm::vec3 scale)
    : id(id), position(position), rotation(rotation), scale(scale) {}

Entity::Entity(std::uint64_t id, const Entity& other)
    : id(id), position(other.position), rotation(other.rotation), scale(other.scale) {}

Component* Entity::addComponent(yyjson_val* componentData) {
  auto it = componentConstructors.find(yyjson_get_str(yyjson_obj_get(componentData, "type")));
  if (it == componentConstructors.end()) return nullptr;
  std::unique_ptr<Component> component = reinterpret_cast<std::unique_ptr<Component>(*)(std::uint64_t, const Entity&, yyjson_val*)>(it->second)(++nextComponentId, *this, componentData);
  return components.emplace(component->getId(), std::move(component)).first->second.get();
}

void Entity::removeComponent(const uint64_t id) {
  components.erase(id);
}
