#include "Shader.hpp"

#include "GraphicsInstance.hpp"
#include "Pipeline.hpp"

#include <volk.h>

#include <fstream>
#include <utility>

Shader::Shader(const GraphicsDevice& device, const std::filesystem::path& path) : device(device), path(absolute(path)) {
  std::ifstream stream(path, std::ios_base::ate | std::ios_base::binary);
  const std::size_t size = static_cast<std::size_t>(stream.tellg());
  stream.seekg(0, std::ios_base::beg);
  contents.resize(size, '\0');
  stream.read(contents.data(), static_cast<std::streamsize>(size));
  compile();
}

Shader::~Shader() {
  delete reflection;
  reflection = nullptr;
  vkDestroyDescriptorSetLayout(device.device, layout, nullptr);
  layout = VK_NULL_HANDLE;
  vkDestroyShaderModule(device.device, module, nullptr);
  module = VK_NULL_HANDLE;
}

void Shader::compile() {
  const shaderc::Compiler compiler;
  const shaderc::CompileOptions options;

  const shaderc::CompilationResult result = compiler.CompileGlslToSpv(contents.c_str(), contents.size(), shaderc_glsl_default_compute_shader, path.string().data(), options);
  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    GraphicsInstance::showError(false, "Failed to compile shader. Error: " + result.GetErrorMessage());
    return;
  }
  std::vector<uint32_t> spv = {result.cbegin(), result.cend()};
  const VkShaderModuleCreateInfo shaderModuleCreateInfo {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,
    .flags = 0,
    .codeSize = spv.size() * sizeof(uint32_t),
    .pCode = spv.data()
  };
  GraphicsInstance::showError(vkCreateShaderModule(device.device, &shaderModuleCreateInfo, nullptr, &module), "Failed to create shader module.");

  // Reflection
  reflection = new spv_reflect::ShaderModule{spv.size() * sizeof(decltype(spv)::value_type), spv.data()};
  spv.clear();
  uint32_t count{};
  GraphicsInstance::showError(reflection->EnumerateDescriptorSets(&count, nullptr) == SPV_REFLECT_RESULT_SUCCESS, "Failed to count descriptor sets.");
  std::vector<SpvReflectDescriptorSet*> reflectedSets{count};
  GraphicsInstance::showError(reflection->EnumerateDescriptorSets(&count, reflectedSets.data()) == SPV_REFLECT_RESULT_SUCCESS, "Failed to enumerate descriptor sets.");
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
  };
  const auto& [_, binding_count, reflectedBinding] = *reflectedSets[0];
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  for (uint32_t i{}; i < binding_count; ++i) {
    bindings.emplace_back(reflectedBinding[i]->binding, static_cast<VkDescriptorType>(reflectedBinding[i]->descriptor_type), 1, static_cast<VkShaderStageFlagBits>(reflection->GetShaderStage()));
    for (uint32_t j{}; j < reflectedBinding[i]->array.dims_count; ++j)
      bindings.back().descriptorCount *= reflectedBinding[i]->array.dims[j];
  }
  descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  descriptorSetLayoutCreateInfo.pBindings    = bindings.data();
  GraphicsInstance::showError(vkCreateDescriptorSetLayout(device.device, &descriptorSetLayoutCreateInfo, nullptr, &layout), "Failed to create descriptor set layout.");
}

VkPipelineBindPoint Shader::getBindPoint() const {
  switch (reflection->GetShaderStage()) {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
    case SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT:
    case SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT:
      return VK_PIPELINE_BIND_POINT_GRAPHICS;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
      return VK_PIPELINE_BIND_POINT_COMPUTE;
    case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
      return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
  }
  return VK_PIPELINE_BIND_POINT_GRAPHICS;
}