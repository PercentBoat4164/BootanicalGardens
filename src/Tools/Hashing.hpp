#pragma once

#include <cinttypes>
#include <functional>
#include <string_view>
#include <string>

namespace Tools {
constexpr std::uint64_t combine(const std::uint64_t a, const std::uint64_t b) { return a ^ b + 0x9e3779b9 + (a << 6) + (a >> 2); }

constexpr std::uint64_t hashString(const std::string_view& str) {
  std::uint64_t hash = 0xcbf29ce484222325;
  for (const char c : str) {
    hash ^= c;
    hash *= 0x00000100000001B3;
  }
  return hash;
}

template<typename T> constexpr std::uint64_t hash(const T& arg) {
  if constexpr (std::is_convertible_v<T, std::string_view>) return Tools::hashString(arg);
  else return std::hash<T>()(arg);
}

template<typename T, typename... Args> constexpr std::uint64_t hash(const T& arg, Args&&... args) {
  return combine(Tools::hash(arg), Tools::hash(std::forward<Args>(args)...));
}
}