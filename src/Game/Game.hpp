#pragma once

#include "src/Entity.hpp"

#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>

class Game {
  static std::unordered_map<std::uint64_t, Entity> entities;
  static double time;
  static double tickTime;
  static std::uint64_t nextEntityId;

public:
  static const std::chrono::steady_clock::time_point startTime;
  /**
   * Constructs an Entity given a set of arguments that Entity is constructible from.
   * @tparam Args
   * @param args
   * @return The new Entity
   */
  template<typename... Args> requires std::constructible_from<Entity, std::uint64_t, Args...> static Entity& addEntity(Args&&... args) {
    ++nextEntityId;
    return entities.emplace(std::piecewise_construct, std::forward_as_tuple(nextEntityId), std::forward_as_tuple(nextEntityId, std::forward<Args&&>(args)...)).first->second;
  }

  /**
   * Move the game state forward one tick.
   */
  static bool tick();

  /**
   * Get the time the last tick took.
   * @return the time in seconds
   */
  static double getTickTime();

  /**
   * Get the total time the game has been running.
   * @return the time in seconds
   */
  static double getTime();
};