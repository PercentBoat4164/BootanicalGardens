#include "src/Entity.hpp"
#include "src/InputEngine/KeyboardController.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Window.hpp"

#include <SDL3/SDL.h>

void buildEntities() {
  Entity entity{};
  entity.addComponent(std::make_shared<KeyboardController>("MainCharacter"));
}

int main() {
  GraphicsInstance::create({});
  {
    buildEntities();

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