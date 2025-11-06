#pragma once

#include <vulkan/vulkan_core.h>

#include <string>
#include <unordered_map>

struct Vertex {
  static std::unordered_map<std::string, VkVertexInputRate> inputRates;
};