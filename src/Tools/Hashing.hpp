#pragma once

#include <functional>
#include <type_traits>

template<typename T> requires std::is_integral_v<T> void combine(T& a, const T b) { a ^= b + 0x9e3779b9 + (a << 6) + (a >> 2); }

template<typename... Args> std::size_t hash(Args... args) {
  std::size_t hash{};
  ([&hash, &args] {combine(hash, std::hash<decltype(args)>{}(args));}(), ...);
  return hash;
}