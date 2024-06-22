#pragma once

#include "src/Component.hpp"

#include <SDL3/SDL_events.h>

#include <functional>
#include <unordered_map>
#include <utility>

template<typename T> struct Hash {
  size_t operator()(const T& x) const {
    size_t hash = 0xcbf29ce484222325;
    for (uint64_t i{}; i < sizeof(T); ++i) {
      /**@todo Properly test me.*/
      hash ^= *static_cast<char*>(&x + 1);
      hash *= 0x100000001b3;
    }
    return hash;
  }
};

class KeyboardController : public Component {
  std::unordered_map<SDL_KeyboardEvent, std::function<void()>, Hash<SDL_KeyboardEvent>> events;

public:
  explicit KeyboardController(std::string name) : Component(std::move(name)) {}
  ~KeyboardController() override = default;
};