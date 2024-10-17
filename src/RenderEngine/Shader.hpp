#pragma once

#include "GraphicsDevice.hpp"

#include <filesystem>

class yyjson_doc;
class yyjson_mut_doc;
class yyjson_val;
class yyjson_mut_val;

class Pipeline;
class GraphicsDevice;
class Resource;
class ShaderUsage;

class Shader {
  const GraphicsDevice& device;
  struct ReadWriteJson {
    bool isMutable;
    union ReadWriteDoc {
      yyjson_mut_doc* const m;
      yyjson_doc* const i;
    } doc;
    union ReadWriteVal {
      yyjson_mut_val* m;
      yyjson_val* i;
    } val;
    ReadWriteJson(yyjson_mut_doc* const doc, yyjson_mut_val* val) : doc(doc), val(val), isMutable(true) {}
    ReadWriteJson(yyjson_doc* const doc, yyjson_val* val) : doc(reinterpret_cast<yyjson_mut_doc* const>(doc)), val(reinterpret_cast<yyjson_mut_val*>(val)), isMutable(false) {}
  } json;

  std::string contents;
  VkShaderStageFlagBits stage{};
  VkPipelineBindPoint bindPoint{};
  VkShaderModule module{VK_NULL_HANDLE};
  std::string entryPoint;
  std::vector<VkDescriptorSetLayout> layouts{};

  friend Pipeline;
  friend ShaderUsage;

public:
  explicit Shader(const GraphicsDevice& device, yyjson_val* obj);
  explicit Shader(const GraphicsDevice& device, yyjson_mut_val* obj);
  ~Shader();

  void buildBlob(const std::filesystem::path& sourcePath);
  [[nodiscard]] VkPipelineBindPoint getBindPoint() const;
};