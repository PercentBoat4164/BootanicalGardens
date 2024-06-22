#pragma once

#include <string>
#include <utility>

class Entity;

class Component {
private:
  std::string name;

public:
  explicit Component(std::string name) : name(std::move(name)) {}
  virtual ~Component() = 0;

  virtual void OnTick(Entity& entity) = 0;

  std::string getName();
};