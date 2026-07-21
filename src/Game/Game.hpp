#pragma once

#include <chrono>

namespace tf { class Executor; class Taskflow; };

class Game {
  static double time;
  static double tickTime;
  static std::uint64_t entityId;
  static tf::Executor executor;

public:
  static const std::chrono::steady_clock::time_point startTime;
  static tf::Taskflow tickGraph;

  /**
   * Move the game state forward one tick. First poll SDL events, then run the tickGraph, then render the scene.
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