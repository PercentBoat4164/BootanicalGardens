#include "Shader.hpp"

#include "src/Tools/StringHash.h"
#include "GraphicsInstance.hpp"
#include "Pipeline.hpp"

#include <volk.h>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <yyjson.h>

#include <fstream>

bool readFile(const std::filesystem::path& path, std::string& contents) {
  std::ifstream stream{path, std::ios_base::ate | std::ios_base::binary};
  if (!stream.is_open()) return false;
  const std::size_t size = static_cast<std::size_t>(stream.tellg());
  stream.seekg(0, std::ios_base::beg);
  contents.resize(size);
  stream.read(contents.data(), static_cast<std::streamsize>(size));
  return true;
}

bool readFile(const std::filesystem::path& path, char*& contents, std::streamsize& contentSize) {
  std::ifstream stream{path, std::ios_base::ate | std::ios_base::binary};
  if (!stream.is_open()) return false;
  contentSize = stream.tellg();
  stream.seekg(0, std::ios_base::beg);
  contents = new char[contentSize];
  stream.read(contents, contentSize);
  return true;
}

class Includer final : public shaderc::CompileOptions::IncluderInterface {
public:
  shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) override {
    const std::filesystem::path path = std::filesystem::canonical(std::filesystem::canonical(requesting_source).parent_path()/requested_source);
    char* contents;
    std::streamsize contentSize;
    readFile(path, contents, contentSize);
    const std::string::size_type sourceLength = path.string().length();
    const auto source = new char[sourceLength];
    memcpy(source, path.c_str(), sourceLength);
    return new shaderc_include_result{source, sourceLength, contents, static_cast<std::size_t>(contentSize), nullptr};
  }

  void ReleaseInclude(shaderc_include_result* data) override {
    delete data->source_name;
    delete data->content;
    delete data;
  }
};

Shader::Shader(const GraphicsDevice& device, yyjson_val* obj) : device(device), json(nullptr, obj) {}

Shader::Shader(const GraphicsDevice& device, yyjson_mut_val* obj) : device(device), json(nullptr, obj) {
  /**@todo Build Vulkan objects from JSON data*/
  /*
  const VkShaderModuleCreateInfo shaderModuleCreateInfo {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = productionCode.size() * sizeof(uint32_t),
      .pCode = productionCode.data()
  };
  GraphicsInstance::showError(vkCreateShaderModule(device.device, &shaderModuleCreateInfo, nullptr, &module), "Failed to create shader module.");

  switch (stage) {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
    case SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT:
    case SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT:
     bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; break;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
     bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE; break;
    case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
    case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
     bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR; break;
    default: return;
  }
   */
}

Shader::~Shader() {
  for (const VkDescriptorSetLayout& layout : layouts)
    vkDestroyDescriptorSetLayout(device.device, layout, nullptr);
  layouts.clear();
  vkDestroyShaderModule(device.device, module, nullptr);
  module = VK_NULL_HANDLE;
}

void Shader::buildBlob(const std::filesystem::path& sourcePath) {
  if (!json.isMutable) return;  /**@todo Log this control path.*/
  const shaderc::Compiler compiler;
  shaderc::CompileOptions options;

  shaderc_shader_kind shaderKind;
  switch (Tools::hash(sourcePath.extension())) {
    case Tools::hash(".hlsl"):
    case Tools::hash(".glsl"): shaderKind = shaderc_glsl_infer_from_source; break;
    case Tools::hash(".vert"): shaderKind = shaderc_glsl_default_vertex_shader; break;
    case Tools::hash(".tesc"): shaderKind = shaderc_glsl_default_tess_control_shader; break;
    case Tools::hash(".tese"): shaderKind = shaderc_glsl_default_tess_evaluation_shader; break;
    case Tools::hash(".geom"): shaderKind = shaderc_glsl_geometry_shader; break;
    case Tools::hash(".frag"): shaderKind = shaderc_glsl_default_fragment_shader; break;
    case Tools::hash(".comp"): shaderKind = shaderc_glsl_default_compute_shader; break;
    case Tools::hash(".mesh"): shaderKind = shaderc_glsl_default_mesh_shader; break;
    case Tools::hash(".task"): shaderKind = shaderc_glsl_default_task_shader; break;
    case Tools::hash(".rgen"): shaderKind = shaderc_glsl_default_raygen_shader; break;
    case Tools::hash(".rint"): shaderKind = shaderc_glsl_default_intersection_shader; break;
    case Tools::hash(".rahit"): shaderKind = shaderc_glsl_default_anyhit_shader; break;
    case Tools::hash(".rchit"): shaderKind = shaderc_glsl_default_closesthit_shader; break;
    case Tools::hash(".rmiss"): shaderKind = shaderc_glsl_default_miss_shader; break;
    default: shaderKind = shaderc_glsl_infer_from_source; break;
  }

  options.SetIncluder(std::make_unique<Includer>());
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  shaderc::CompilationResult result = compiler.CompileGlslToSpv(contents.c_str(), contents.size(), shaderKind, sourcePath.string().data(), options);
  if (result.GetCompilationStatus() != shaderc_compilation_status_success) GraphicsInstance::showError(false, "Failed to compile production shader. Error: " + result.GetErrorMessage());

  yyjson_mut_val* codeStorage = yyjson_mut_obj_get(json.val.m, "code");
  for (uint32_t i : result) yyjson_mut_arr_add_uint(json.doc.m, codeStorage, i);

  options.SetOptimizationLevel(shaderc_optimization_level_zero);
  options.SetGenerateDebugInfo();
  result = compiler.CompileGlslToSpv(contents.c_str(), contents.size(), shaderKind, sourcePath.string().data(), options);
  std::vector code = {result.cbegin(), result.cend()};

  const auto* reflectionData = new spv_reflect::ShaderModule{code.size() * sizeof(decltype(code)::value_type), code.data(), SPV_REFLECT_MODULE_FLAG_NO_COPY};

  stage      = static_cast<VkShaderStageFlagBits>(reflectionData->GetShaderStage());
  entryPoint = reflectionData->GetEntryPointName();

  uint32_t count{};
  // reflect the vertex format
  GraphicsInstance::showError(reflectionData->EnumerateInputVariables(&count, nullptr) == SPV_REFLECT_RESULT_SUCCESS, "Failed to count input variables.");
  std::vector<SpvReflectInterfaceVariable*> variables{count};
  GraphicsInstance::showError(reflectionData->EnumerateInputVariables(&count, variables.data()) == SPV_REFLECT_RESULT_SUCCESS, "Failed to enumerate input variables.");
  for (const auto variable : variables) {
    if (variable->built_in > SpvBuiltInMax) ;
  }

  // reflect the descriptor layout
  GraphicsInstance::showError(reflectionData->EnumerateDescriptorSets(&count, nullptr) == SPV_REFLECT_RESULT_SUCCESS, "Failed to count descriptor sets.");
  std::vector<SpvReflectDescriptorSet*> reflectedSets{count};
  GraphicsInstance::showError(reflectionData->EnumerateDescriptorSets(&count, reflectedSets.data()) == SPV_REFLECT_RESULT_SUCCESS, "Failed to enumerate descriptor sets.");
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
  };
  layouts.resize(count, VK_NULL_HANDLE);
  for (uint64_t i{}; i < count; ++i) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (uint32_t j{}; j < reflectedSets[i]->binding_count; ++j) {
      bindings.emplace_back(reflectedSets[i]->bindings[j]->binding, static_cast<VkDescriptorType>(reflectedSets[i]->bindings[j]->descriptor_type), 1, stage);
      for (uint32_t k{}; k < reflectedSets[i]->bindings[j]->array.dims_count; ++k)
        bindings.back().descriptorCount *= reflectedSets[i]->bindings[j]->array.dims[k];
    }
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptorSetLayoutCreateInfo.pBindings    = bindings.data();
    GraphicsInstance::showError(vkCreateDescriptorSetLayout(device.device, &descriptorSetLayoutCreateInfo, nullptr, &layouts[i]), "Failed to create descriptor set layout.");
  }
}

VkPipelineBindPoint Shader::getBindPoint() const {
  return bindPoint;
}