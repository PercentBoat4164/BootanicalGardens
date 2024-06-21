#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>


class GraphicsDevice;
class Renderer;
class Shader;
class Resource;

class ShaderUsage {
public:
  const Shader& shader;
  std::unordered_map<std::string, Resource*> resources;
  VkDescriptorSet set{VK_NULL_HANDLE};

  explicit ShaderUsage(const GraphicsDevice& device, const Shader& shader, std::unordered_map<std::string, Resource*> resources, const Renderer& renderer);
};
