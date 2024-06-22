#pragma once

#include <chrono>

class Time {
  static std::chrono::steady_clock::time_point startTime;
  static double time;

public:
  Time() = delete;

  static bool tick();
  static double tickTime();
};