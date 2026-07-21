#include "Game.hpp"

#include "src/InputEngine/Input.hpp"
#include <taskflow.hpp>

double Game::time{};
double Game::tickTime{std::numeric_limits<double>::infinity()};
std::uint64_t Game::entityId{UINT64_MAX};
tf::Executor Game::executor{};
tf::Taskflow Game::tickGraph{};

const std::chrono::steady_clock::time_point Game::startTime{std::chrono::steady_clock::now()};

bool Game::tick() {
  bool shouldQuit{};
  SDL_Event e;
  while (!shouldQuit && SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_EVENT_QUIT: shouldQuit = true; break;
        // Input Events
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP: Input::onEvent(e); break;
      default: break;
    }
  }
  double currentTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
  tickTime           = currentTime - time;
  time               = currentTime;
  Input::onTick();
  executor.run(tickGraph).wait();
  return !shouldQuit;
}

double Game::getTickTime() {
  return tickTime;
}

double Game::getTime() {
  return time;
}