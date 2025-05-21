#pragma once

#include "Json.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Tools {
template<>
glm::vec3 jsonGet(yyjson_val* jsonData, std::string_view key) {
  glm::vec3 out;
  jsonData = yyjson_obj_get(jsonData, key.data());
  if (jsonData == nullptr) return {};
  assert(yyjson_arr_size(jsonData) == 3);
  out.x = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 0)));
  out.y = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 1)));
  out.z = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 2)));
  return out;
}

template<>
glm::quat jsonGet(yyjson_val* jsonData, std::string_view key) {
  glm::quat out;
  jsonData = yyjson_obj_get(jsonData, key.data());
  if (jsonData == nullptr) return {};
  assert(yyjson_arr_size(jsonData) == 4);
  out.x = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 0)));
  out.y = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 1)));
  out.z = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 2)));
  out.w = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 3)));
  return out;
}
}  // namespace Tools
