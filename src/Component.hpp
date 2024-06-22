#pragma once

#include <string>
#include <utility>

class Component {
private:
  std::string name;

public:
  explicit Component(std::string name) : name(std::move(name)) {}
  virtual ~Component() = 0;

  std::string getName() {
    return name;
  }
};