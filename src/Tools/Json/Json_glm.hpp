#pragma once

#include "Json.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Tools {
template<> inline glm::vec3 jsonGet(yyjson_val* jsonData) {
  glm::vec3 out;
  if (jsonData == nullptr) return {};
  assert(yyjson_arr_size(jsonData) == 3);
  out.x = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 0)));
  out.y = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 1)));
  out.z = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 2)));
  return out;
}

template<> inline glm::quat jsonGet(yyjson_val* jsonData) {
  glm::quat out;
  if (jsonData == nullptr) return {};
  assert(yyjson_arr_size(jsonData) == 4);
  out.x = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 0)));
  out.y = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 1)));
  out.z = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 2)));
  out.w = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 3)));
  return out;
}

template<> inline glm::mat4 jsonGet(yyjson_val* jsonData) {
  glm::mat4 out;
  if (jsonData == nullptr) return {};
  assert(yyjson_arr_size(jsonData) == 4);
  yyjson_val* arr;
  int i, max;
  yyjson_arr_foreach(jsonData, i, max, arr) {
    assert(yyjson_arr_size(arr) == 4);
    out[i].x = static_cast<float>(yyjson_get_num(yyjson_arr_get(arr, 0)));
    out[i].y = static_cast<float>(yyjson_get_num(yyjson_arr_get(arr, 1)));
    out[i].z = static_cast<float>(yyjson_get_num(yyjson_arr_get(arr, 2)));
    out[i].w = static_cast<float>(yyjson_get_num(yyjson_arr_get(arr, 3)));
  }
  return out;
}
}  // namespace Tools
