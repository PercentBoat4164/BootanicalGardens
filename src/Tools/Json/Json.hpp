#pragma once

#include "yyjson.h"

#include <string_view>

namespace Tools {
template<typename T> T jsonGet(yyjson_val* jsonData) {
  static_assert(false, "Please use a specialization of the jsonGet function.");
}
}  // namespace Tools
