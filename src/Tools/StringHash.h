#pragma once

#include <cstdint>
#include <string_view>

namespace Tools {
constexpr uint32_t hash(const char *arr) {
  uint64_t hash = 0xcbf29ce484222325;
  while (*arr != 0) {
    hash ^= *arr++;
    hash *= 0x00000100000001B3;
  }
  return static_cast<uint32_t>((hash & 0xffffffff00000000) >> 32 ^ hash);
}

constexpr uint32_t hash(const std::string_view arr) {
  uint64_t hash = 0xcbf29ce484222325;
  for (const char c : arr) {
    hash ^= c;
    hash *= 0x00000100000001B3;
  }
  return static_cast<uint32_t>((hash & 0xffffffff00000000) >> 32 ^ hash);
}

constexpr uint32_t hash(const std::string& arr) {
  uint64_t hash = 0xcbf29ce484222325;
  for (const char c : arr) {
    hash ^= c;
    hash *= 0x00000100000001B3;
  }
  return static_cast<uint32_t>((hash & 0xffffffff00000000) >> 32 ^ hash);
}
}