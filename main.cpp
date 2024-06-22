#include "src/Entity.hpp"
#include "src/Game/Time.hpp"
#include "src/InputEngine/KeyboardController.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Window.hpp"

void buildEntities() {
  Entity entity{};
  entity.addComponent(std::make_shared<KeyboardController>("MainCharacter"));
}

int main() {
  GraphicsInstance::create({});
  {
    const GraphicsDevice graphicsDevice{};
    {
      Window window{graphicsDevice};

      do {
        window.draw();
      } while (Time::tick());
    }
  }
  GraphicsInstance::destroy();
  return 0;
}