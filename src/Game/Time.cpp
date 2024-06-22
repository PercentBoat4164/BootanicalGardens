#include "Time.hpp"

#include "src/InputEngine/Input.hpp"

#include <chrono>

std::chrono::steady_clock::time_point Time::startTime{std::chrono::steady_clock::now()};
double Time::time{};

bool Time::tick() {
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

double Time::tickTime() {
  return time;
}
