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
      case SDL_QUIT: shouldQuit = true; break;
        // Input Events
      case SDL_KEYDOWN:
      case SDL_KEYUP: Input::onEvent(e); break;
    }
  }
  time = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
  Input::onTick();
  return !shouldQuit;
}

double Game::getTime() {
  return time;
}