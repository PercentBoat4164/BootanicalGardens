#include "Shader.hpp"

#include "src/Tools/StringHash.h"
#include "GraphicsInstance.hpp"
#include "Pipeline.hpp"

#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#include <volk/volk.h>
#include <yyjson.h>

#include <fstream>

bool readFile(const std::filesystem::path& path, std::string& contents) {
  std::ifstream stream{path, std::ios_base::ate | std::ios_base::binary};
  if (!stream.is_open()) return false;
  const std::size_t size = stream.tellg();
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
    const std::filesystem::path path = canonical(std::filesystem::canonical(requesting_source).parent_path()/requested_source);
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

Shader::Shader(const std::shared_ptr<GraphicsDevice>& device, yyjson_val* obj) : device(device) {
  // Source file path
  sourcePath = yyjson_get_str(yyjson_obj_get(obj, "source"));

  // Compiled shader code
  yyjson_val* json_code = yyjson_obj_get(obj, "code");
  code.resize(yyjson_arr_size(json_code));
  size_t idx, max;
  yyjson_val * val;
  yyjson_arr_foreach(json_code, idx, max, val) code[idx] = static_cast<uint32_t>(yyjson_get_int(val));
  const VkShaderModuleCreateInfo shaderModuleCreateInfo {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = code.size() * sizeof(uint32_t),
      .pCode = code.data()
  };
  if (const VkResult result = vkCreateShaderModule(device->device, &shaderModuleCreateInfo, nullptr, &module); result != VK_SUCCESS) GraphicsInstance::showError(result, "Failed to create shader module.");

  // Shader stage and bind point
  stage = static_cast<VkShaderStageFlagBits>(yyjson_get_int(yyjson_obj_get(obj, "stage")));

  switch (static_cast<SpvReflectShaderStageFlagBits>(stage)) {
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

  // "main" entry point
  bool has_main = false;
  yyjson_val* entryPoints = yyjson_obj_get(obj, "entry points");
  yyjson_val* mainEntryPoint;
  yyjson_arr_foreach(entryPoints, idx, max, mainEntryPoint)
    if (strcmp(yyjson_get_str(yyjson_obj_get(mainEntryPoint, "name")), "main") == 0) {
      has_main = true;
      break;
    }
  if (!has_main) return;
}

Shader::Shader(const std::shared_ptr<GraphicsDevice>& device, const std::filesystem::path& sourcePath) : device(device), sourcePath(sourcePath) {
  const shaderc::Compiler compiler;
  shaderc::CompileOptions options;
  options.SetIncluder(std::make_unique<Includer>());
  shaderc_shader_kind shaderKind;
  switch (Tools::hash(sourcePath.extension().string())) {
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

  std::string contents;
  if (!readFile(sourcePath, contents)) GraphicsInstance::showError("failed to read file: '" + sourcePath.string() + "'");

  // Compile with debug info and no optimizations to get reflection data
  options.SetOptimizationLevel(shaderc_optimization_level_zero);
  options.SetGenerateDebugInfo();
  shaderc::CompilationResult compilationResult = compiler.CompileGlslToSpv(contents.c_str(), contents.size(), shaderKind, sourcePath.string().data(), options);
  code = {compilationResult.cbegin(), compilationResult.cend()};

  reflectionData = spv_reflect::ShaderModule{code.size() * sizeof(decltype(code)::value_type), code.data(), SPV_REFLECT_MODULE_FLAG_NO_COPY};
  stage      = static_cast<VkShaderStageFlagBits>(reflectionData.GetShaderStage());
  entryPoint = reflectionData.GetEntryPointName();

  // Compile with optimal performance to build the pipeline with
  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  compilationResult = compiler.CompileGlslToSpv(contents.c_str(), contents.size(), shaderKind, sourcePath.string().data(), options);
  if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success) GraphicsInstance::showError(compilationResult.GetErrorMessage());
  code = {compilationResult.cbegin(), compilationResult.cend()};

  const VkShaderModuleCreateInfo shaderModuleCreateInfo {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = code.size() * sizeof(decltype(code)::value_type),
      .pCode = code.data()
  };
  if (const VkResult result = vkCreateShaderModule(device->device, &shaderModuleCreateInfo, nullptr, &module); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create shader module");
}

Shader::~Shader() {
  vkDestroyShaderModule(device->device, module, nullptr);
  module = VK_NULL_HANDLE;
}

void Shader::save(yyjson_mut_doc* doc, yyjson_mut_val* obj) const {
  // Start fresh
  yyjson_mut_obj_clear(obj);
  
  // Source path
  yyjson_mut_obj_add_strcpy(doc, obj, "source", sourcePath.string().c_str());
  
  // Performance optimized code
  yyjson_mut_obj_add(obj, yyjson_mut_strcpy(doc, "code"), yyjson_mut_arr_with_sint32(doc, reinterpret_cast<const int32_t*>(code.data()), code.size()));
  
  // Shader stage
  yyjson_mut_obj_add_int(doc, obj, "stage", stage);
  
  // Entry points
  yyjson_mut_val* entryPoints = yyjson_mut_obj_add_arr(doc, obj, "entry points");
  yyjson_mut_val* epObj = yyjson_mut_arr_add_obj(doc, entryPoints);
  // Entry point name
  yyjson_mut_obj_add_strcpy(doc, epObj, "name", entryPoint.c_str());
}

VkShaderStageFlagBits Shader::getStage() const { return stage; }
VkPipelineBindPoint Shader::getBindPoint() const { return bindPoint; }
VkShaderModule Shader::getModule() const { return module; }
std::string_view Shader::getEntryPoint() const { return entryPoint; }
const spv_reflect::ShaderModule* Shader::getReflectedData() const { return &reflectionData; }