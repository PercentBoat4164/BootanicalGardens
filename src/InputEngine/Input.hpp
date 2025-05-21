#pragma once

#include <SDL3/SDL_events.h>

#include <unordered_map>

class Game;

class Input {
  static std::unordered_map<SDL_Keycode, float> keys;

  static void onEvent(SDL_Event event);
  static void onTick();
  friend Game;

public:
  Input() = delete;

  static bool initialize();

  static bool keyPressed(SDL_Keycode key);
  static float keyDown(SDL_Keycode key);
  static bool keyReleased(SDL_Keycode key);
  static float keyUp(SDL_Keycode key);
};
