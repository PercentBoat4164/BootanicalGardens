#pragma once

#include "Component.hpp"
#include "Tools/ClassName.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <ranges>
#include <vector>

struct yyjson_val;

class Entity {
  static std::unordered_map<std::string, void*> componentConstructors;

  std::unordered_map<uint64_t, std::unique_ptr<Component>> components;
  uint64_t nextComponentId{UINT64_MAX};
  std::uint64_t id;

public:
  glm::vec3 position;
  glm::dquat rotation;
  glm::vec3 scale;

  /**
   * Registers a new Component Constructor to enable the use of <c>addComponent</c> for a new Component type.
   * If another Component Constructor of the same <c>name</c> has already been registered this function will return <c>false</c>
   * and the old Constructor will not be overridden.
   * @param name the name of the function. to be used as a parameter of <c>addComponent</c>
   * @param function the functon to be called
   * @return <c>false</c> if a pre-existing Component Constructor would have been overridden, <c>true</c> otherwise.
   */
  static bool registerComponentConstructor(const std::string& name, void* function);

  /**
   * Constructor an empty Entity with the given position, rotation, and scale.
   * @param position the initial position of the entity
   * @param rotation the initial orientation
   * @param scale the initial scale
   */
  explicit Entity(std::uint64_t id, glm::vec3 position = glm::vec3(), glm::dquat rotation = glm::dquat(), glm::vec3 scale = glm::vec3());

  /**
   * Constructs an empty Entity with the same position, rotation, and scale as the given Entity.
   * @param other the entity this is based on
   */
  explicit Entity(std::uint64_t id, const Entity& other);
  Entity(const Entity& other)      = default;
  Entity(Entity&& other) noexcept  = default;
  Entity& operator=(const Entity&) = default;
  Entity& operator=(Entity&&)      = default;
  ~Entity()                        = default;

  /**
   * Add a Component to the map of Components.
   * @tparam T Derives from Component
   * @param component The Component to be added
   */
  Component* addComponent(yyjson_val* componentData);

  /**
   * Add a Component to the map of Components.
   * @tparam T Derives from Component
   * @param args The arguments for the Component's constructor
   */
  template<typename T, typename... Args> requires std::constructible_from<T, uint64_t, const Entity&, Args...> && std::derived_from<T, Component> void addComponent(Args&& ... args) {
    std::unique_ptr<Component> component = reinterpret_cast<std::unique_ptr<Component>(*)(std::uint64_t, const Entity&, Args...)>(componentConstructors.at(static_cast<std::string>(Tools::className<T>())))(++nextComponentId, *this, std::forward<Args>(args)...);
    components.emplace(component->getId(), std::move(component));
  }

  /**
   * Remove a Component from the map of Components.
   * @param id The id of the Component to be removed
   */
  void removeComponent(uint64_t id);

  /**
   * Remove a Component from the map of Components.
   * @tparam T Derives from Component
   * @param component The Component to be added
   */
  template<typename T> requires std::derived_from<T, Component> void removeComponent(std::shared_ptr<T> component) {
    components.erase(components.find(component));
  }

  /**
   * Get a shared pointer to a Component object in this Entity.
   * @tparam T Derives from Component
   * @return A shared pointer to the Component
   */
  template<typename T> requires std::derived_from<T, Component> T* getComponent(uint64_t name) {
    auto it = components.find(name);
    if (it == components.end()) return nullptr;
    return *it->second;
  }

  template<typename T> requires std::derived_from<T, Component> T* getComponentOfType() {
    for (std::unique_ptr<Component>& component : std::ranges::views::values(components))
      if (dynamic_cast<T*>(component.get()) != nullptr) return component;
    return nullptr;
  }

  /**
   * Get a vector of shared pointers of all Components of a specific type in this Entity.
   * @tparam T Derives from Component
   * @return a Vector containing shared pointers of all Components of a specific type in this Entity
   */
  template<typename T> requires std::derived_from<T, Component> std::vector<T*> getComponentsOfType() {
    std::vector<T*> requestedComponents;
    for (std::unique_ptr<Component>& component : std::ranges::views::values(components))
      if (dynamic_cast<T*>(component.get()) != nullptr) requestedComponents.push_back(component);
    return requestedComponents;
  }
};