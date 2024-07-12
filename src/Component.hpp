#pragma once

#include <string>
#include <utility>
#include <memory>

class Entity;

class Component {
private:
  std::string name;

public:
  std::shared_ptr<Entity> entity;

  explicit Component(std::string name, std::shared_ptr<Entity>& entity) : name(std::move(name)), entity(std::move(entity)) {}
  virtual ~Component() = 0;

  virtual void onTick() = 0;

  std::string getName();
};