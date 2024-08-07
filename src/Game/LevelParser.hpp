#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <simdjson.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

class Entity;
class Component;

/**
 * Loads levels from level files to be used by the Game class
 */
class LevelParser {
private:
  static simdjson::ondemand::parser parser;

public:
  /**
   * Loads an entity from the .json file.
   *
   * @param name The name of the entity in the .json file
   * @return A pointer to the newly-loaded entity
   */
  static Entity& loadEntity(simdjson::ondemand::object entityData);

  /**
   * Loads an entire level from the .json file
   */
  static void loadLevel(const std::filesystem::path& filename);
};