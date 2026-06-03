#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>

class Game {
  //static std::unordered_map<std::uint64_t, Entity> entities;
  static double time;
  static double tickTime;
  static std::uint64_t entityId;

public:
  static const std::chrono::steady_clock::time_point startTime;

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