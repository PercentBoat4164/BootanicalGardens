#pragma once

#include "src/Filesystem/UpdateListener.hpp"
#include "src/Game/Game.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Renderable/Renderable.hpp"
#include "src/RenderEngine/Window.hpp"

#include <iostream>
#include <functional>


int main() {
  std::function<void()> deleteNotice = []() { std::cout << "File deleted" << std::endl; };
  std::function<void()> modifyNotice = []() { std::cout << "File modified" << std::endl; };
  UpdateListener::addDirectoryWatch("../res", nullptr, deleteNotice, modifyNotice, nullptr);
  //UpdateListener::addDirectoryWatch("modelPath", nullptr, nullptr, nullptr, nullptr);
  UpdateListener::startWatching();

  while(true) {

  }

  /*GraphicsInstance::create({});
  {
    const GraphicsDevice graphicsDevice{};
    // Renderable renderable(graphicsDevice, std::filesystem::path("../res/FlightHelmet.glb"));
    // Shader shader{graphicsDevice};
    // shader.buildBlob(std::filesystem::canonical("../res/basic.frag"));

    // Window window{graphicsDevice};
    //
    // do {
    //   window.draw();
    // } while (Game::tick());
  }
  GraphicsInstance::destroy();*/

  return 0;
}