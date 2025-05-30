#pragma once

#include "GraphicsDevice.hpp"

#include <SPIRV-Reflect/spirv_reflect.h>
#include <filesystem>

class yyjson_val;
class yyjson_mut_val;
class yyjson_mut_doc;

class Pipeline;
class GraphicsDevice;
class Resource;

class Shader {
  const std::shared_ptr<GraphicsDevice> device;

  std::filesystem::path sourcePath;
  std::vector<uint32_t> code;  /**@todo: Avoid keeping the shader code in memory.*/
  VkShaderStageFlagBits stage{};
  VkPipelineBindPoint bindPoint{};
  VkShaderModule module{VK_NULL_HANDLE};
  std::string entryPoint;
  spv_reflect::ShaderModule reflectionData;

public:
  explicit Shader(const std::shared_ptr<GraphicsDevice>& device, yyjson_val* obj);
  explicit Shader(const std::shared_ptr<GraphicsDevice>& device, const std::filesystem::path& sourcePath);
  ~Shader();

  void save(yyjson_mut_doc* doc, yyjson_mut_val* obj) const;

  [[nodiscard]] VkShaderStageFlagBits getStage() const;
  [[nodiscard]] VkPipelineBindPoint getBindPoint() const;
  [[nodiscard]] VkShaderModule getModule() const;
  [[nodiscard]] std::string_view getEntryPoint() const;
  [[nodiscard]] const spv_reflect::ShaderModule* getReflectedData() const;
};