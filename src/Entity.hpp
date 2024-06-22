#pragma once

#include "Component.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <ranges>
#include <vector>

class Entity {
  std::unordered_map<std::string, std::shared_ptr<Component>> components;
  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;

public:
  /**
   * Add a Component to the map of Components
   *
   * @tparam T Derives from Component
   * @param component The Component to be added
   */
  template<typename T> requires std::derived_from<T, Component> void addComponent(std::shared_ptr<T> component) {
    components.emplace(component->getName(), component);
  }

  /**
   * Remove a Component from the map of Components
   *
   * @param name The name of the Component to be removed. A variable of the Component
   */
  void removeComponent(const std::string& name);

  /**
   * Remove a Component from the map of Components
   *
   * @tparam T Derives from Component
   * @param component The Component to be added
   */
  template<typename T> requires std::derived_from<T, Component> void removeComponent(std::shared_ptr<T> component) {
    components.erase(components.find(component));
  }

  /**
   * Get a shared pointer to a Component object in this Entity
   *
   * @tparam T Derives from Component
   * @return A shared pointer to the Component
   */
  template<typename T> requires std::derived_from<T, Component> std::shared_ptr<T> getComponent(const std::string& name) {
    auto it = components.find(name);
    if (it == components.end()) return nullptr;
    return *it;
  }

  /**
   * Get a vector of shared pointers of all Components of a specific type in this Entity
   *
   * @tparam T Derives from Component
   * @return a Vector containing shared pointers of all Components of a specific type in this Entity
   */
  template<typename T> requires std::derived_from<T, Component> std::vector<std::shared_ptr<T>> getComponentsOfType() {
    std::vector<std::shared_ptr<T>> requestedComponents;
    for (std::shared_ptr<Component>& component : std::ranges::views::values(components))
      if (dynamic_cast<T*>(component.get()) != nullptr) requestedComponents.push_back(component);
    return requestedComponents;
  }
};