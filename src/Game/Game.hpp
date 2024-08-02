#pragma once

#include <chrono>
#include <memory>
#include <vector>

class Entity;

class Game {
  static std::vector<std::shared_ptr<Entity>> entities;
  static double time;

public:
  static const std::chrono::steady_clock::time_point startTime;
  static std::shared_ptr<Entity> addEntity();

  /**
   * Move the game state forward one tick.
   */
  static bool tick();

  /**
   * Get the total time the game has been running.
   * @return the time in seconds
   */
  static double getTime();
};