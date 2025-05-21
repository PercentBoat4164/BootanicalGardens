#include "Input.hpp"

#include "src/Game/Game.hpp"

#include <SDL3/SDL.h>
#include <ranges>

std::unordered_map<SDL_Keycode, float> Input::keys{};

/**
 * Handle an event. Should be called for every event
 * relevant to Input.
 *
 * @param event The event to be handled
 */
void Input::onEvent(SDL_Event event) {
  SDL_Keycode code = event.key.key;
  if (event.type == SDL_EVENT_KEY_DOWN) keys[code] = 0;
  else if (event.type == SDL_EVENT_KEY_UP) keys[code] = -0;
}

/**
 * Update input values that change every tick
 */
void Input::onTick() {
  for (float& key : std::ranges::views::values(keys)) key += Game::getTime() * std::abs(key) == key ? 1 : -1;
}

/**
 * Initialize SDL's event system
 *
 * @return <c>true</c> on success, <c>false</c> on failure.
 */
bool Input::initialize() {
  return SDL_InitSubSystem(SDL_INIT_EVENTS);
}

/**
 * Get whether a key has been pressed in the last tick
 *
 * @param key The key to check
 * @return whether the key has been pressed
 */
bool Input::keyPressed(const SDL_Keycode key) {
  return keys[key] == Game::getTime();
}

/**
 * Get the time in seconds that a key has been pressed. If 0 is
 * returned, the key is up.
 *
 * @param key the key to check
 * @return The time in seconds
 */
float Input::keyDown(const SDL_Keycode key) {
  const float value = keys[key];
  return value >= 0 ? value : 0;
}

/**
 * Get whether the key has been released in the last tick
 *
 * @param key the key to check
 * @return whether the key has been released
 */
bool Input::keyReleased(const SDL_Keycode key) {
  return keys[key] == -Game::getTime();
}

/**
 *Get the time in seconds that a key has been released
 *
 * @param key the key to check
 * @return the time in seconds
 */
float Input::keyUp(const SDL_Keycode key) {
  const float value = keys[key];
  return value <= -0 ? value : 0;
}