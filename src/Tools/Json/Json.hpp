#pragma once

#include "yyjson.h"

#include <string_view>

namespace Tools {
template<typename T> T jsonGet(yyjson_val* jsonData, std::string_view key) { static_assert("Unknown specialization."); return {}; }
}  // namespace Tools
