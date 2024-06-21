#pragma once

#include "GraphicsDevice.hpp"

#include "external/spirv-reflect/7eb5db58882afa602ffe790c1183057596a2dab1/spirv_reflect.h"
#include <shaderc/shaderc.hpp>

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