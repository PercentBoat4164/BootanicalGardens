#include "src/Entity.hpp"
#include "src/Game/Components/PlayerController.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Window.hpp"
#include "src/Game/Game.hpp"
#include "src/Game/LevelParser.hpp"

#include <iostream>

int main() {
  LevelParser::loadLevel("../res/levels/Level1.json");
  std::cout << "done";
//  GraphicsInstance::create({});
//  {
//    const GraphicsDevice graphicsDevice{};
//    Window window{graphicsDevice};
//
//    do {
//      window.draw();
//    } while (Game::tick());
//  }
//  GraphicsInstance::destroy();
//  return 0;
}