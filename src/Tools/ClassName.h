#pragma once

#include <string_view>

namespace Tools {
template<typename T> consteval std::string_view className() {
  const std::string_view name = typeid(T).name();
  return name.substr(6, name.find_first_of('<') - 6);
}
}