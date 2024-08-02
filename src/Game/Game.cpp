#include "Game.hpp"

#include "src/InputEngine/Input.hpp"

std::unordered_map<std::uint64_t, Entity> Game::entities{};
double Game::time{};
std::uint64_t Game::nextEntityId{UINT64_MAX};

const std::chrono::steady_clock::time_point Game::startTime{std::chrono::steady_clock::now()};

bool Game::tick() {
  bool shouldQuit{};
  SDL_Event e;
  while (!shouldQuit && SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_EVENT_QUIT: shouldQuit = true; break;
        // Input Events
      case SDL_EVENT_KEYBOARD_ADDED:
      case SDL_EVENT_KEYBOARD_REMOVED:
      case SDL_EVENT_KEYMAP_CHANGED:
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP: Input::onEvent(e); break;
    }
  }

  time = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
  Input::onTick();
  return !shouldQuit;
}

double Game::getTime() {
  return time;
}