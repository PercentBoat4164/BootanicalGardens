#pragma once

#include "src/Game/Game.hpp"
#include "src/Game/LevelParser.hpp"
#include "src/InputEngine/Input.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Renderable/Renderable.hpp"
#include "src/RenderEngine/Window.hpp"


#include <chrono>

int main() {

  LevelParser::loadLevel(std::filesystem::canonical("../res/levels/Level1.json"));
  GraphicsInstance::create({});
  {
    const GraphicsDevice graphicsDevice{};
     //Renderable renderable(graphicsDevice, std::filesystem::path("../res/FlightHelmet.glb"));
     //Shader shader{graphicsDevice};
    // shader.buildBlob(std::filesystem::canonical("../res/basic.frag"));

    Window window{graphicsDevice};
    while (Game::tick()) {
      //if (Input::keyPressed(Input::W)) std::cout << "W Pressed!" << std::endl;
      //else if (Input::keyReleased(Input::W)) std::cout << "W Released!" << std::endl;
      //std::cout << Input::keys[Input::W] << " | " << Input::keyDown(Input::W) << std::endl;
      //std::this_thread::sleep_for(std::chrono::milliseconds(10));
   }
  }
  GraphicsInstance::destroy();

  return 0;
}