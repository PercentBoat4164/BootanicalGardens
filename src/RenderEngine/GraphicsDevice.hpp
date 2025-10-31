#pragma once

#include "yyjson.h"
#include "src/RenderEngine/DescriptorSetAllocator.hpp"
#include "src/RenderEngine/Pipeline/VertexProcess.hpp"

#include <VkBootstrap.h>
#include <vma/vk_mem_alloc.h>


#include <filesystem>
#include <unordered_map>

struct FragmentProcess;
class Shader;
class Texture;
class Material;
class Mesh;
class Pipeline;
class CommandBuffer;

class GraphicsDevice {
public:
  vkb::Device device;
  VkQueue globalQueue;
  uint32_t globalQueueFamilyIndex;
  VmaAllocator allocator{VK_NULL_HANDLE};
  VkCommandPool commandPool{VK_NULL_HANDLE};
  DescriptorSetAllocator descriptorSetAllocator{*this};

  std::unordered_map<std::uint64_t, VkSampler> samplers;
  std::unordered_map<std::uint64_t, FragmentProcess> fragmentProcesses;
  std::unordered_map<std::uint64_t, VertexProcess> vertexProcesses;
  std::unordered_map<std::uint64_t, std::unique_ptr<Shader>> shaders;
  std::unordered_map<std::uint64_t, std::shared_ptr<Texture>> textures;
  std::unordered_map<std::uint64_t, Pipeline> pipelines;
  std::unordered_map<std::uint64_t, Material> overrideMaterials;
  std::unordered_map<std::uint64_t, Material> materials;
  std::unordered_map<std::uint64_t, Mesh> meshes;

  yyjson_doc* graphicsJSON;
  std::filesystem::path resourcesDirectory;
  yyjson_val* JSONTextureArray;
  std::uint64_t JSONTextureArrayCount;
  yyjson_val* JSONShaderArray;
  std::uint64_t JSONShaderArrayCount;
  yyjson_val* JSONOverrideProcesses;
  yyjson_val* JSONFragmentProcessArray;
  std::uint64_t JSONFragmentProcessArrayCount;
  yyjson_val* JSONVertexProcessArray;
  std::uint64_t JSONVertexProcessArrayCount;
  yyjson_val* JSONMaterialArray;
  std::uint64_t JSONMaterialArrayCount;
  yyjson_val* JSONMeshArray;
  std::uint64_t JSONMeshArrayCount;

  explicit GraphicsDevice(const std::filesystem::path& path);
  ~GraphicsDevice();

  VkSampler* getSampler(VkFilter magnificationFilter=VK_FILTER_NEAREST, VkFilter minificationFilter=VK_FILTER_NEAREST, VkSamplerMipmapMode mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST, VkSamplerAddressMode addressMode=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, float lodBias=0, VkBorderColor borderColor=VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
  FragmentProcess* getJSONFragmentProcess(const std::string& name);
  FragmentProcess* getJSONFragmentProcess(std::uint64_t id);
  VertexProcess* getJSONVertexProcess(const std::string& name);
  VertexProcess* getJSONVertexProcess(std::uint64_t id);
  Shader* getJSONShader(std::uint64_t id);
  std::weak_ptr<Texture> getJSONTexture(std::uint64_t id);
  Pipeline* getPipeline(Material* material, std::uint64_t renderPassCompatibility);
  Material* getMaterial(std::uint64_t id, const Material* material);
  Material* getMaterial(std::uint64_t id);
  Mesh* getJSONMesh(std::uint64_t id);

  void update();

  struct ImmediateExecutionContext {
    VkCommandBuffer commandBuffer;
    VkFence fence;
  };

  [[nodiscard]] ImmediateExecutionContext executeCommandBufferAsync(const CommandBuffer& commandBuffer) const;
  void waitForAsyncCommandBuffer(ImmediateExecutionContext context) const;
  void executeCommandBufferImmediate(const CommandBuffer& commandBuffer) const;
};
