#include "LevelParser.hpp"
#include "Game.hpp"
#include "src/Entity.hpp"

#include <yyjson.h>

#include <filesystem>

yyjson_doc* LevelParser::doc{nullptr};

Entity& LevelParser::loadEntity(yyjson_val* entityData) {
  std::array<double, 3> position{};
  yyjson_val* jsonData = yyjson_obj_get(entityData, "position");
  if (yyjson_arr_size(jsonData) == 3) {
    position[0] = yyjson_get_num(yyjson_arr_get(jsonData, 0));
    position[1] = yyjson_get_num(yyjson_arr_get(jsonData, 1));
    position[2] = yyjson_get_num(yyjson_arr_get(jsonData, 2));
  }
  std::array<double, 4> rotation {};
  jsonData = yyjson_obj_get(entityData, "rotation");
  if (yyjson_arr_size(jsonData) == 4) {
    rotation[0] = yyjson_get_num(yyjson_arr_get(jsonData, 0));
    rotation[1] = yyjson_get_num(yyjson_arr_get(jsonData, 1));
    rotation[2] = yyjson_get_num(yyjson_arr_get(jsonData, 2));
    rotation[3] = yyjson_get_num(yyjson_arr_get(jsonData, 3));
  }
  std::array<double, 4> scale {};
  jsonData = yyjson_obj_get(entityData, "scale");
  if (yyjson_arr_size(jsonData) == 3) {
    scale[0] = yyjson_get_num(yyjson_arr_get(jsonData, 0));
    scale[1] = yyjson_get_num(yyjson_arr_get(jsonData, 1));
    scale[2] = yyjson_get_num(yyjson_arr_get(jsonData, 2));
  }
  Entity& entity = Game::addEntity(
      glm::vec3(position[0], position[1], position[2]),
      glm::dquat(rotation[0], rotation[1], rotation[2], rotation[3]),
      glm::vec3(scale[0], scale[1], scale[2])
  );

  yyjson_val* components = yyjson_obj_get(entityData, "components");
  for (uint32_t i = 0; i < yyjson_get_len(components); ++i) {
    entity.addComponent(yyjson_arr_get(components, i));
  }

  return entity;
}

void LevelParser::loadLevel(const std::filesystem::path& filename) {
  yyjson_read_err error;
  doc = yyjson_read_file(filename.c_str(), YYJSON_READ_ALLOW_INF_AND_NAN, nullptr, &error);
  if (doc == nullptr) { /**@todo: Read `error`.*/ }
}