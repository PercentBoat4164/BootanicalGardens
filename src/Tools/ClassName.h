#pragma once

#include <string_view>

namespace Tools {
template<typename T>
std::string_view className() {
  std::string_view name = typeid(T).name();
  return name.substr(6, name.find_first_of('<') - 6);
}
}