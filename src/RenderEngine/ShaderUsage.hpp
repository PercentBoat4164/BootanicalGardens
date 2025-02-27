#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>


class GraphicsDevice;
class RenderGraph;
class Shader;
class Resource;

class ShaderUsage {
public:
  const Shader& shader;
  std::unordered_map<std::string, Resource&> resources;
  VkDescriptorSet set{VK_NULL_HANDLE};

  explicit ShaderUsage(const GraphicsDevice& device, const Shader& shader, const std::unordered_map<std::string, Resource&>& resources, const RenderGraph& renderer);
};
