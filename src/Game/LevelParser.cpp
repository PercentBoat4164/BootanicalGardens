#include "LevelParser.hpp"
#include "Game.hpp"
#include "src/Entity.hpp"
#include "src/Tools/Json/Json.hpp"

#include <yyjson.h>

#include <filesystem>

yyjson_doc* LevelParser::doc{nullptr};

Entity& LevelParser::loadEntity(yyjson_val* entityData) {
  auto position  = Tools::jsonGet<glm::vec3>(entityData, "position");
  auto rotation  = Tools::jsonGet<glm::quat>(entityData, "rotation");
  auto scale     = Tools::jsonGet<glm::vec3>(entityData, "scale");
  Entity& entity = Game::addEntity(position, rotation, scale);

  yyjson_val* components = yyjson_obj_get(entityData, "components");
  for (uint32_t i = 0; i < yyjson_get_len(components); ++i) {
    entity.addComponent(yyjson_arr_get(components, i));
  }

  return entity;
}

void LevelParser::loadLevel(const std::filesystem::path& filename) {
  yyjson_read_err error;
  doc = yyjson_read_file(filename.string().c_str(), YYJSON_READ_ALLOW_INF_AND_NAN, nullptr, &error);
  if (doc == nullptr) { /**@todo Read `error`.*/ }
}