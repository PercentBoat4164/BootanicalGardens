#pragma once

#include "GraphicsDevice.hpp"

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

#include <filesystem>


class Pipeline;
class GraphicsDevice;
class Resource;

class Shader {
  const GraphicsDevice& device;

  std::string contents;
  spv_reflect::ShaderModule* reflection;

public:
  std::filesystem::path path;
  VkShaderModule module{VK_NULL_HANDLE};
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};

  explicit Shader(const GraphicsDevice& device, const std::filesystem::path& path);
  ~Shader();

  void compile();
  [[nodiscard]] VkPipelineBindPoint getBindPoint() const;
};