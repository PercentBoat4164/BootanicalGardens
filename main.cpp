#include "src/Entity.hpp"
#include "src/Game/Components/PlayerController.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Window.hpp"
#include "src/Game/Game.hpp"
#include "simdjson.h"

int main() {
  GraphicsInstance::create({});
  {
    const GraphicsDevice graphicsDevice{};
    Window window{graphicsDevice};

    do {
      window.draw();
    } while (Game::tick());
  }
  GraphicsInstance::destroy();
  return 0;
}