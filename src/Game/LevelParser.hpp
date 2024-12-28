#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

struct yyjson_doc;
struct yyjson_val;

class Entity;
class Component;

/**
 * Loads levels from level files to be used by the Game class
 */
class LevelParser {
private:
    static yyjson_doc* doc;
public:
  /**
   * Loads an entity from the .json file.
   *
   * @param name The name of the entity in the .json file
   * @return A pointer to the newly-loaded entity
   */
  static Entity& loadEntity(yyjson_val* entityData);

  /**
   * Loads an entire level from the .json file
   *
   * @param filename The path to the .json file
   */
  static void loadLevel(const std::filesystem::path& filename);
};