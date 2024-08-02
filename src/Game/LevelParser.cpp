#include "LevelParser.hpp"
#include "Game.hpp"
#include "src/Entity.hpp"

#include <simdjson.h>

#include <filesystem>

simdjson::ondemand::parser LevelParser::parser{};

Entity& LevelParser::loadEntity(simdjson::ondemand::object entityData) {
  simdjson::ondemand::array position = entityData["position"];
  auto it = position.begin();
  double x = *it;
  double y = *++it;
  double z = *++it;
  //x = ;
  //  simdjson::ondemand::array rotation = entityData["rotation"];
//  simdjson::ondemand::array scale = entityData["scale"];
  Entity& entity = Game::addEntity(
      //glm::vec3(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)),
      //glm::dquat(static_cast<double>(rotation.at(0)), static_cast<double>(rotation.at(1)), static_cast<double>(rotation.at(2)), static_cast<double>(rotation.at(3))),
      //glm::vec3(static_cast<double>(scale.at(0)), static_cast<double>(scale.at(1)), static_cast<double>(scale.at(2)))
  );

  for (simdjson::ondemand::object componentData : entityData["components"])
    entity.addComponent(componentData);

  return entity;
}

void LevelParser::loadLevel(const std::filesystem::path& filename) {
  simdjson::padded_string contents = simdjson::padded_string::load(filename.string());
  simdjson::ondemand::document doc = parser.iterate(contents);
  simdjson::ondemand::array entities = doc["entities"];
  for (simdjson::ondemand::object entity : entities)
    loadEntity(entity);
}