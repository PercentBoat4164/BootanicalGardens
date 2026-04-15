#pragma once

#include "../GraphicsDevice.hpp"

#include <spirv_reflect.h>
#include <filesystem>

struct yyjson_mut_doc;
struct yyjson_mut_val;

class Pipeline;
class GraphicsDevice;
class Resource;

class Shader {
  GraphicsDevice* const device;

  std::filesystem::path sourcePath;
  std::vector<uint32_t> code;  /**@todo: Avoid keeping the shader code in memory.*/
  VkShaderStageFlagBits stage;
  VkPipelineBindPoint bindPoint{};
  VkShaderModule module{VK_NULL_HANDLE};
  std::string entryPoint;
  spv_reflect::ShaderModule reflectionData;

public:
  explicit Shader(GraphicsDevice* device, yyjson_val* obj);
  explicit Shader(GraphicsDevice* device, const std::filesystem::path& sourcePath, std::vector<std::pair<std::string, std::string>> definitions = {});
  ~Shader();

  static std::unique_ptr<Shader> jsonGet(GraphicsDevice* graphicsDevice, yyjson_val* jsonData);

  void save(yyjson_mut_doc* doc, yyjson_mut_val* obj) const;

  [[nodiscard]] VkShaderStageFlagBits getStage() const;
  [[nodiscard]] VkPipelineBindPoint getBindPoint() const;
  [[nodiscard]] VkShaderModule getModule() const;
  [[nodiscard]] std::string_view getEntryPoint() const;
  [[nodiscard]] const spv_reflect::ShaderModule* getReflectedData() const;
  [[nodiscard]] bool usesDescriptorSet(std::uint32_t set) const;
};