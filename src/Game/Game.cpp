#include "Game.hpp"

#include "src/InputEngine/Input.hpp"

std::vector<std::shared_ptr<Entity>> Game::entities;
double Game::time;

const std::chrono::steady_clock::time_point Game::startTime{std::chrono::steady_clock::now()};

std::shared_ptr<Entity> Game::addEntity() {
  std::shared_ptr<Entity> entity = entities.emplace_back();
  return entity;
}

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

double Game::tickTime() {
  return time;
}