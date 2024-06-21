#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Window.hpp"

#include <SDL3/SDL.h>

int main() {
  GraphicsInstance::create({});
  {
    const GraphicsDevice graphicsDevice{};
    Window window{graphicsDevice};

    SDL_Event e;
    do {
      while (SDL_PollEvent(&e)) if (e.type == SDL_EVENT_QUIT) break;

      window.draw();
    } while(e.type != SDL_EVENT_QUIT);
  }
  GraphicsInstance::destroy();
  return 0;
}

/*
  res
  |-- models
  |   |-- ghost.gltf
  |
  |-- sounds
  |   |-- boo.wav
  |
  |-- levels
      |-- japanese.level  // JSON that can be used to generate a group of Entities

  src
  |-- Component.cpp
  |-- Entity.cpp  // Manages a group of Components
  |-- RenderEngine
  |   |-- Renderable.cpp  // A model, material, textures, and other rendering parameters | Inherits from Component.cpp
  |
  |-- Game
  |   |-- Level.cpp  // Manages loading and storing data relevant to levels
  |   |-- Components
  |       |-- Enemy.cpp  // Controls behaviour of enemies | Inherits from Component.cpp
  |       |-- Player.cpp  // Controls behaviour of player | Inherits from Component.cpp
  |       |-- Npc.cpp  // Controls behaviour of npcs | Inherits from Component.cpp
  |
  |-- Audio
      |-- Noisy.cpp  // A sound, and other audio parameters | Inherits from Component.cpp
*/