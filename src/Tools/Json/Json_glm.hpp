#pragma once

#include "Json.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>

namespace Tools {
template<>
glm::vec3 jsonGet(yyjson_val* jsonData, std::string_view key);

template<>
glm::quat jsonGet(yyjson_val* jsonData, std::string_view key);

template<>
glm::mat4 jsonGet(yyjson_val* jsonData, std::string_view key);
}  // namespace Tools
