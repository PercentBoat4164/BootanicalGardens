#include "src/GraphicsDevice.hpp"
#include "src/GraphicsInstance.hpp"
#include "src/Window.hpp"

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
