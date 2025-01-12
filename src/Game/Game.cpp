#include "Game.hpp"

#include "src/InputEngine/Input.hpp"

std::unordered_map<std::uint64_t, Entity> Game::entities{};
double Game::time{};
double Game::tickTime{};
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
  double currentTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
  tickTime           = currentTime - time;
  time = currentTime;
  Input::onTick();

  //call onTick for every component in the game
  //todo: move to systems?
  for (auto& entityPair : entities) {
    for(const std::shared_ptr<Component>& component : entityPair.second.getComponents()) {
      component->onTick();
    }
  }
  return !shouldQuit;
}

double Game::getTickTime() {
  return tickTime;
}

double Game::getTime() {
  return time;
}