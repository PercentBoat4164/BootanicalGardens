#include "Input.hpp"

#include "src/Game/Game.hpp"

#include <SDL2/SDL.h>
#include <ranges>

std::unordered_map<Input::KeyCode, float> Input::keys{};

/**
 * Handle an event. Should be called for every event
 * relevant to Input.
 *
 * @param event The event to be handled
 */
void Input::onEvent(SDL_Event& event) {
  KeyCode code;
  switch (event.key.keysym.sym) {
      case SDLK_UNKNOWN: code = UNKNOWN; break;
      case SDLK_RETURN: code = RETURN; break;
      case SDLK_ESCAPE: code = ESCAPE; break;
      case SDLK_BACKSPACE: code = BACKSPACE; break;
      case SDLK_TAB: code = TAB; break;
      case SDLK_SPACE: code = SPACE; break;
      case SDLK_EXCLAIM: code = EXCLAIM; break;
//      case SDLK_DBLAPOSTROPHE: code = DOUBLE_APOSTROPHE; break;
      case SDLK_HASH: code = HASH; break;
      case SDLK_DOLLAR: code = DOLLAR; break;
//      case SDLK_PERCENT: code = PERCENT; break;
      case SDLK_AMPERSAND: code = AMPERSAND; break;
//      case SDLK_APOSTROPHE: code = APOSTROPHE; break;
      case SDLK_LEFTPAREN: code = LEFT_PARENTHESIS; break;
      case SDLK_RIGHTPAREN: code = RIGHT_PARENTHESIS; break;
      case SDLK_ASTERISK: code = ASTERISK; break;
      case SDLK_PLUS: code = PLUS; break;
      case SDLK_COMMA: code = COMMA; break;
      case SDLK_MINUS: code = MINUS; break;
      case SDLK_PERIOD: code = PERIOD; break;
      case SDLK_SLASH: code = SLASH; break;
      case SDLK_0: code = ZERO; break;
      case SDLK_1: code = ONE; break;
      case SDLK_2: code = TWO; break;
      case SDLK_3: code = THREE; break;
      case SDLK_4: code = FOUR; break;
      case SDLK_5: code = FIVE; break;
      case SDLK_6: code = SIX; break;
      case SDLK_7: code = SEVEN; break;
      case SDLK_8: code = EIGHT; break;
      case SDLK_9: code = NINE; break;
      case SDLK_COLON: code = COLON; break;
      case SDLK_SEMICOLON: code = SEMICOLON; break;
      case SDLK_LESS: code = LESS; break;
      case SDLK_EQUALS: code = EQUALS; break;
      case SDLK_GREATER: code = GREATER; break;
      case SDLK_QUESTION: code = QUESTION; break;
      case SDLK_AT: code = AT; break;
//      case SDLK_A: code = A; break;
//      case SDLK_B: code = B; break;
//      case SDLK_C: code = C; break;
//      case SDLK_D: code = D; break;
//      case SDLK_E: code = E; break;
//      case SDLK_F: code = F; break;
//      case SDLK_G: code = G; break;
//      case SDLK_H: code = H; break;
//      case SDLK_I: code = I; break;
//      case SDLK_J: code = J; break;
//      case SDLK_K: code = K; break;
//      case SDLK_L: code = L; break;
//      case SDLK_M: code = M; break;
//      case SDLK_N: code = N; break;
//      case SDLK_O: code = O; break;
//      case SDLK_P: code = P; break;
//      case SDLK_Q: code = Q; break;
//      case SDLK_R: code = R; break;
//      case SDLK_S: code = S; break;
//      case SDLK_T: code = T; break;
//      case SDLK_U: code = U; break;
//      case SDLK_V: code = V; break;
//      case SDLK_W: code = W; break;
//      case SDLK_X: code = X; break;
//      case SDLK_Y: code = Y; break;
//      case SDLK_Z: code = Z; break;
      case SDLK_LEFTBRACKET: code = LEFT_BRACKET; break;
      case SDLK_BACKSLASH: code = BACKSLASH; break;
      case SDLK_RIGHTBRACKET: code = RIGHT_BRACKET; break;
      case SDLK_CARET: code = CARET; break;
      case SDLK_UNDERSCORE: code = UNDERSCORE; break;
//      case SDLK_GRAVE: code = GRAVE; break;
      case SDLK_a: code = A; break;
      case SDLK_b: code = B; break;
      case SDLK_c: code = C; break;
      case SDLK_d: code = D; break;
      case SDLK_e: code = E; break;
      case SDLK_f: code = F; break;
      case SDLK_g: code = G; break;
      case SDLK_h: code = H; break;
      case SDLK_i: code = I; break;
      case SDLK_j: code = J; break;
      case SDLK_k: code = K; break;
      case SDLK_l: code = L; break;
      case SDLK_m: code = M; break;
      case SDLK_n: code = N; break;
      case SDLK_o: code = O; break;
      case SDLK_p: code = P; break;
      case SDLK_q: code = Q; break;
      case SDLK_r: code = R; break;
      case SDLK_s: code = S; break;
      case SDLK_t: code = T; break;
      case SDLK_u: code = U; break;
      case SDLK_v: code = V; break;
      case SDLK_w: code = W; break;
      case SDLK_x: code = X; break;
      case SDLK_y: code = Y; break;
      case SDLK_z: code = Z; break;
//      case SDLK_LEFTBRACE: code = LEFT_BRACE; break;
//      case SDLK_PIPE: code = PIPE; break;
//      case SDLK_RIGHTBRACE: code = RIGHT_BRACE; break;
//      case SDLK_TILDE: code = TILDE; break;
      case SDLK_DELETE: code = DELETE; break;
      case SDLK_CAPSLOCK: code = CAPSLOCK; break;
      case SDLK_F1: code = F1; break;
      case SDLK_F2: code = F2; break;
      case SDLK_F3: code = F3; break;
      case SDLK_F4: code = F4; break;
      case SDLK_F5: code = F5; break;
      case SDLK_F6: code = F6; break;
      case SDLK_F7: code = F7; break;
      case SDLK_F8: code = F8; break;
      case SDLK_F9: code = F9; break;
      case SDLK_F10: code = F10; break;
      case SDLK_F11: code = F11; break;
      case SDLK_F12: code = F12; break;
//      case SDLK_PRINTSCREEN: code = PRINT_SCREEN; break;
//      case SDLK_SCROLLOCK: code = SCROLL_LOCK; break;
      case SDLK_PAUSE: code = PAUSE; break;
      case SDLK_INSERT: code = INSERT; break;
      case SDLK_HOME: code = HOME; break;
      case SDLK_PAGEUP: code = PAGEUP; break;
      case SDLK_END: code = END; break;
      case SDLK_PAGEDOWN: code = PAGEDOWN; break;
      case SDLK_RIGHT: code = RIGHT; break;
      case SDLK_LEFT: code = LEFT; break;
      case SDLK_DOWN: code = DOWN; break;
      case SDLK_UP: code = UP; break;
//      case SDLK_NUMLOCKCLEAR: code = NUMLOCK_CLEAR; break;
      case SDLK_KP_DIVIDE: code = KEYPAD_DIVIDE; break;
      case SDLK_KP_MULTIPLY: code = KEYPAD_MULTIPLY; break;
      case SDLK_KP_MINUS: code = KEYPAD_MINUS; break;
      case SDLK_KP_PLUS: code = KEYPAD_PLUS; break;
      case SDLK_KP_ENTER: code = KEYPAD_ENTER; break;
//      case SDLK_KP1: code = KEYPAD_1; break;
//      case SDLK_KP2: code = KEYPAD_2; break;
//      case SDLK_KP3: code = KEYPAD_3; break;
//      case SDLK_KP4: code = KEYPAD_4; break;
//      case SDLK_KP5: code = KEYPAD_5; break;
//      case SDLK_KP6: code = KEYPAD_6; break;
//      case SDLK_KP7: code = KEYPAD_7; break;
//      case SDLK_KP8: code = KEYPAD_8; break;
//      case SDLK_KP9: code = KEYPAD_9; break;
//      case SDLK_KP0: code = KEYPAD_0; break;
      case SDLK_KP_PERIOD: code = KEYPAD_PERIOD; break;
//      case SDLK_APPLICATION: code = APPLICATION; break;
      case SDLK_POWER: code = POWER; break;
      case SDLK_KP_EQUALS: code = KEYPAD_EQUALS; break;
      case SDLK_F13: code = F13; break;
      case SDLK_F14: code = F14; break;
      case SDLK_F15: code = F15; break;
//      case SDLK_F16: code = F16; break;
//      case SDLK_F17: code = F17; break;
//      case SDLK_F18: code = F18; break;
//      case SDLK_F19: code = F19; break;
//      case SDLK_F20: code = F20; break;
//      case SDLK_F21: code = F21; break;
//      case SDLK_F22: code = F22; break;
//      case SDLK_F23: code = F23; break;
//      case SDLK_F24: code = F24; break;
//      case SDLK_EXECUTE: code = EXECUTE; break;
//      case SDLK_HELP: code = HELP; break;
//      case SDLK_MENU: code = MENU; break;
//      case SDLK_SELECT: code = SELECT; break;
//      case SDLK_STOP: code = STOP; break;
//      case SDLK_AGAIN: code = AGAIN; break;
//      case SDLK_UNDO: code = UNDO; break;
//      case SDLK_CUT: code = CUT; break;
//      case SDLK_COPY: code = COPY; break;
//      case SDLK_PASTE: code = PASTE; break;
//      case SDLK_FIND: code = FIND; break;
//      case SDLK_MUTE: code = MUTE; break;
//      case SDLK_VOLUMEUP: code = VOLUME_UP; break;
//      case SDLK_VOLUMEDOWN: code = VOLUME_DOWN; break;
//      case SDLK_KP_COMMA: code = KEYPAD_COMMA; break;
//      case SDLK_KP_EQUALSAS400: code = KEYPAD_EQUALSAS400; break;
//      case SDLK_ALTERASE: code = ALT_ERASE; break;
//      case SDLK_SYSREQ: code = SYSTEM_REQUEST; break;
//      case SDLK_CANCEL: code = CANCEL; break;
//      case SDLK_CLEAR: code = CLEAR; break;
//      case SDLK_PRIOR: code = PRIOR; break;
//      case SDLK_RETURN2: code = RETURN2; break;
//      case SDLK_SEPARATOR: code = SEPARATOR; break;
//      case SDLK_OUT: code = OUT; break;
//      case SDLK_OPER: code = OPER; break;
//      case SDLK_CLEARAGAIN: code = CLEAR_AGAIN; break;
//      case SDLK_CRSEL: code = CRSEL; break;
//      case SDLK_EXSEL: code = EXSEL; break;
//      case SDLK_KP_00: code = KEYPAD_00; break;
//      case SDLK_KP_000: code = KEYPAD_000; break;
//      case SDLK_THOUSANDSSEPARATOR: code = THOUSANDS_SEPARATOR; break;
//      case SDLK_DECIMALSEPARATOR: code = DECIMAL_SEPARATOR; break;
//      case SDLK_CURRENCYUNIT: code = CURRENCY_UNIT; break;
//      case SDLK_CURRENCYSUBUNIT: code = CURRENCY_SUBUNIT; break;
//      case SDLK_KP_LEFTPAREN: code = KEYPAD_LEFT_PARENTHESIS; break;
//      case SDLK_KP_RIGHTPAREN: code = KEYPAD_RIGHT_PARENTHESIS; break;
//      case SDLK_KP_LEFTBRACE: code = KEYPAD_LEFT_BRACE; break;
//      case SDLK_KP_RIGHTBRACE: code = KEYPAD_RIGHT_BRACE; break;
//      case SDLK_KP_TAB: code = KEYPAD_TAB; break;
//      case SDLK_KP_BACKSPACE: code = KEYPAD_BACKSPACE; break;
//      case SDLK_KP_A: code = KEYPAD_A; break;
//      case SDLK_KP_B: code = KEYPAD_B; break;
//      case SDLK_KP_C: code = KEYPAD_C; break;
//      case SDLK_KP_D: code = KEYPAD_D; break;
//      case SDLK_KP_E: code = KEYPAD_E; break;
//      case SDLK_KP_F: code = KEYPAD_F; break;
//      case SDLK_KP_XOR: code = KEYPAD_XOR; break;
//      case SDLK_KP_POWER: code = KEYPAD_POWER; break;
//      case SDLK_KP_PERCENT: code = KEYPAD_PERCENT; break;
//      case SDLK_KP_LESS: code = KEYPAD_LESS; break;
//      case SDLK_KP_GREATER: code = KEYPAD_GREATER; break;
//      case SDLK_KP_AMPERSAND: code = KEYPAD_AMPERSAND; break;
//      case SDLK_KP_DBLAMPERSAND: code = KEYPAD_DOUBLE_AMPERSAND; break;
//      case SDLK_KP_VERTICALBAR: code = KEYPAD_VERTICAL_BAR; break;
//      case SDLK_KP_DBLVERTICALBAR: code = KEYPAD_DOUBLE_VERTICAL_BAR; break;
//      case SDLK_KP_COLON: code = KEYPAD_COLON; break;
//      case SDLK_KP_HASH: code = KEYPAD_HASH; break;
//      case SDLK_KP_SPACE: code = KEYPAD_SPACE; break;
//      case SDLK_KP_AT: code = KEYPAD_AT; break;
//      case SDLK_KP_EXCLAM: code = KEYPAD_EXCLAIMATION_POINT; break;
//      case SDLK_KP_MEMSTORE: code = KEYPAD_MEM_STORE; break;
//      case SDLK_KP_MEMRECALL: code = KEYPAD_MEM_RECALL; break;
//      case SDLK_KP_MEMCLEAR: code = KEYPAD_MEM_CLEAR; break;
//      case SDLK_KP_MEMADD: code = KEYPAD_MEM_ADD; break;
//      case SDLK_KP_MEMSUBTRACT: code = KEYPAD_MEM_SUBTRACT; break;
//      case SDLK_KP_MEMMULTIPLY: code = KEYPAD_MEM_MULTIPLY; break;
//      case SDLK_KP_MEMDIVIDE: code = KEYPAD_MEM_DIVIDE; break;
//      case SDLK_KP_PLUSMINUS: code = KEYPAD_PLUS_MINUS; break;
//      case SDLK_KP_CLEAR: code = KEYPAD_CLEAR; break;
//      case SDLK_KP_CLEARENTRY: code = KEYPAD_CLEAR_ENTRY; break;
//      case SDLK_KP_BINARY: code = KEYPAD_BINARY; break;
//      case SDLK_KP_OCTAL: code = KEYPAD_OCTAL; break;
//      case SDLK_KP_DECIMAL: code = KEYPAD_DECIMAL; break;
//      case SDLK_KP_HEXADECIMAL: code = KEYPAD_HEXADECIMAL; break;
//      case SDLK_LCTRL: code = LEFT_CONTROL; break;
//      case SDLK_LSHIFT: code = LEFT_SHIFT; break;
//      case SDLK_LALT: code = LEFT_ALT; break;
//      case SDLK_LGUI: code = LEFT_GUI; break;
//      case SDLK_RCTRL: code = RIGHT_CONTROL; break;
//      case SDLK_RSHIFT: code = RIGHT_SHIFT; break;
//      case SDLK_RALT: code = RIGHT_ALT; break;
//      case SDLK_RGUI: code = RIGHT_GUI; break;
//      case SDLK_MODE: code = MODE; break;
//      case SDLK_SLEEP: code = SLEEP; break;
//      case SDLK_WAKE: code = WAKE; break;
//      case SDLK_CHANNEL_INCREMENT: code = CHANNEL_INCREMENT; break;
//      case SDLK_CHANNEL_DECREMENT: code = CHANNEL_DECREMENT; break;
//      case SDLK_MEDIA_PLAY: code = MEDIA_PLAY; break;
//      case SDLK_MEDIA_PAUSE: code = MEDIA_PAUSE; break;
//      case SDLK_MEDIA_RECORD: code = MEDIA_RECORD; break;
//      case SDLK_MEDIA_FAST_FORWARD: code = MEDIA_FAST_FORWARD; break;
//      case SDLK_MEDIA_REWIND: code = MEDIA_REWIND; break;
//      case SDLK_MEDIA_NEXT_TRACK: code = MEDIA_NEXT_TRACK; break;
//      case SDLK_MEDIA_PREVIOUS_TRACK: code = MEDIA_PREVIOUS_TRACK; break;
//      case SDLK_MEDIA_STOP: code = MEDIA_STOP; break;
//      case SDLK_MEDIA_EJECT: code = MEDIA_EJECT; break;
//      case SDLK_MEDIA_PLAY_PAUSE: code = MEDIA_PLAY_PAUSE; break;
//      case SDLK_MEDIA_SELECT: code = MEDIA_SELECT; break;
//      case SDLK_AC_NEW: code = BROWSER_NEW; break;
//      case SDLK_AC_OPEN: code = BROWSER_OPEN; break;
//      case SDLK_AC_CLOSE: code = BROWSER_CLOSE; break;
//      case SDLK_AC_EXIT: code = BROWSER_EXIT; break;
//      case SDLK_AC_SAVE: code = BROWSER_SAVE; break;
//      case SDLK_AC_PRINT: code = BROWSER_PRINT; break;
//      case SDLK_AC_PROPERTIES: code = BROWSER_PROPERTIES; break;
//      case SDLK_AC_SEARCH: code = BROWSER_SEARCH; break;
//      case SDLK_AC_HOME: code = BROWSER_HOME; break;
//      case SDLK_AC_BACK: code = BROWSER_BACK; break;
//      case SDLK_AC_FORWARD: code = BROWSER_FORWARD; break;
//      case SDLK_AC_STOP: code = BROWSER_STOP; break;
//      case SDLK_AC_REFRESH: code = BROWSER_REFRESH; break;
//      case SDLK_AC_BOOKMARKS: code = BROWSER_BOOKMARKS; break;
//      case SDLK_SOFTLEFT: code = SOFT_LEFT; break;
//      case SDLK_SOFTRIGHT: code = SOFT_RIGHT; break;
//      case SDLK_CALL: code = CALL; break;
//      case SDLK_ENDCALL: code = END_CALL; break;
  }
  if (event.type == SDL_KEYDOWN) keys[code] = 0;
  else if (event.type == SDL_KEYUP) keys[code] = -0;
}

/**
 * Update input values that change every tick
 */
void Input::onTick() {
  for (float& key : std::ranges::views::values(keys)) key += Game::getTime() * std::abs(key) == key ? 1 : -1;
}

/**
 * Get whether a key has been pressed in the last tick
 *
 * @param key The key to check
 * @return whether the key has been pressed
 */
bool Input::keyPressed(KeyCode key) {
  return keys[key] == Game::getTime();
}

/**
 * Get the time in seconds that a key has been pressed. If 0 is
 * returned, the key is up.
 *
 * @param key the key to check
 * @return The time in seconds
 */
float Input::keyDown(KeyCode key) {
  float value = keys[key];
  return value >= 0 ? value : 0;
}

/**
 * Get whether the key has been released in the last tick
 *
 * @param key the key to check
 * @return whether the key has been released
 */
bool Input::keyReleased(KeyCode key) {
  return keys[key] == -Game::getTime();
}

/**
 *Get the time in seconds that a key has been released
 *
 * @param key the key to check
 * @return the time in seconds
 */
float Input::keyUp(KeyCode key) {
  float value = keys[key];
  return value <= -0 ? value : 0;
}