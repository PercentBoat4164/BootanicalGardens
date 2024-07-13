#pragma once

#include "ShaderUsage.hpp"

#include <vector>


class GraphicsDevice;
class Pipeline {
  const GraphicsDevice& device;
  std::vector<ShaderUsage*> shaderUsages;
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};

public:
  VkPipelineBindPoint bindPoint;

  Pipeline(const GraphicsDevice& device, const std::vector<ShaderUsage*>& shaderUsages);
  ~Pipeline();

  void execute(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z) const;
};