#include "Material.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/MeshGroup/Texture.hpp"
#include "src/RenderEngine/Pipeline.hpp"

#include "yyjson.h"
#include "src/RenderEngine/MeshGroup/Vertex.hpp"

#include <ranges>

Material::Material(GraphicsDevice* device, yyjson_val* json, const std::uint32_t id) : device(device), id(id) {
  name = yyjson_get_str(yyjson_obj_get(json, "name"));
  yyjson_val* val = yyjson_obj_get(json, "vertexShader");
  if (yyjson_is_uint(val)) vertexShader = device->getJSONShader(yyjson_get_uint(val));
  val = yyjson_obj_get(json, "fragmentShader");
  if (yyjson_is_uint(val)) fragmentShader = device->getJSONShader(yyjson_get_uint(val));
  doubleSided = yyjson_get_bool(yyjson_obj_get(json, "doubleSided"));
  alphaMode = static_cast<fastgltf::AlphaMode>(yyjson_get_uint(yyjson_obj_get(json, "alphaMode")));
  alphaCutoff = static_cast<float>(yyjson_get_real(yyjson_obj_get(json, "alphaCutoff")));
  val = yyjson_obj_get(json, "albedoTexture");
  if (yyjson_is_uint(val)) albedoTexture = device->getJSONTexture(yyjson_get_uint(val));
  val = yyjson_obj_get(json, "normalTexture");
  if (yyjson_is_uint(val)) normalTexture = device->getJSONTexture(yyjson_get_uint(val));
}

const std::unordered_map<uint32_t, Material::Binding>* Material::getBindings(const uint8_t set) const {
  if (const auto it = perSetBindings.find(set); it != perSetBindings.end()) return &it->second;
  return nullptr;
}

VkPipelineStageFlags getPipelineStages(const VkShaderStageFlags shaderStages) {
  VkPipelineStageFlags pipelineStages = 0;
  if (shaderStages & VK_SHADER_STAGE_VERTEX_BIT) pipelineStages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  if (shaderStages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) pipelineStages |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
  if (shaderStages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) pipelineStages |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
  if (shaderStages & VK_SHADER_STAGE_GEOMETRY_BIT) pipelineStages |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
  if (shaderStages & VK_SHADER_STAGE_FRAGMENT_BIT) pipelineStages |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  if (shaderStages & VK_SHADER_STAGE_COMPUTE_BIT) pipelineStages |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  if (shaderStages & VK_SHADER_STAGE_ALL_GRAPHICS) pipelineStages |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
  if (shaderStages & VK_SHADER_STAGE_ALL) pipelineStages |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
#if VK_KHR_ray_query | VK_KHR_ray_tracing_pipeline
  if (shaderStages & VK_SHADER_STAGE_RAYGEN_BIT_KHR ||
      shaderStages & VK_SHADER_STAGE_ANY_HIT_BIT_KHR ||
      shaderStages & VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR ||
      shaderStages & VK_SHADER_STAGE_MISS_BIT_KHR ||
      shaderStages & VK_SHADER_STAGE_INTERSECTION_BIT_KHR ||
      shaderStages & VK_SHADER_STAGE_CALLABLE_BIT_KHR)
    pipelineStages |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
#endif
#if VK_EXT_mesh_shader
  if (shaderStages & VK_SHADER_STAGE_TASK_BIT_EXT) pipelineStages |= VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT;
  if (shaderStages & VK_SHADER_STAGE_MESH_BIT_EXT) pipelineStages |= VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT;
#endif
  return pipelineStages;
}

std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> Material::computeColorAttachmentAccesses() const {
  const spv_reflect::ShaderModule* reflectedData = fragmentShader->getReflectedData();
  uint32_t count;
  reflectedData->EnumerateOutputVariables(&count, nullptr);
  std::vector<SpvReflectInterfaceVariable*> vars(count);
  reflectedData->EnumerateOutputVariables(&count, vars.data());
  std::map<uint32_t, std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> framebufferAttachments;
  for (const SpvReflectInterfaceVariable* output: vars) {
    framebufferAttachments.emplace(
      output->location,
      std::pair {
        RenderGraph::getImageId(output->name),
        RenderGraph::ImageAccess{
          .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
          .access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
        }
      }
    );
  }
  std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> results;
  results.insert_range(results.begin(), framebufferAttachments | std::ranges::views::values);
  return results;
}

std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> Material::computeInputAttachmentAccesses() const {
  const std::vector shaders = {vertexShader, fragmentShader};

  std::map<uint32_t, std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> inputAttachments;
  for (uint32_t i{}; i < shaders.size(); ++i) {
    std::vector<SpvReflectDescriptorSet*> sets;
    std::vector<SpvReflectInterfaceVariable*> vars;
    const Shader& shader = *shaders[i];
    const spv_reflect::ShaderModule* reflectedData = shader.getReflectedData();
    uint32_t count;
    reflectedData->EnumerateDescriptorSets(&count, nullptr);
    sets.resize(count);
    reflectedData->EnumerateDescriptorSets(&count, sets.data());
    for (const auto* set : sets) {
      for (uint32_t j{}; j < set->binding_count; ++j) {
        const auto* binding = set->bindings[j];
        if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) continue;
        inputAttachments.emplace(
          binding->binding,
          std::pair {
            RenderGraph::getImageId(binding->name),
            RenderGraph::ImageAccess {
              .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
              .usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
              .access = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
              .stage = getPipelineStages(shader.getStage()),
              .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
              .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
              .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
              .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            }
          }
        );
      }
    }
  }
  std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> results;
  results.insert_range(results.begin(), inputAttachments | std::ranges::views::values);
  return results;
}

std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> Material::computeBoundImageAccesses() const {
  const std::vector shaders = {vertexShader, fragmentShader};

  std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> results;
  for (uint32_t i{}; i < shaders.size(); ++i) {
    std::vector<SpvReflectDescriptorSet*> sets;
    std::vector<SpvReflectInterfaceVariable*> vars;
    const Shader& shader = *shaders[i];
    const spv_reflect::ShaderModule* reflectedData = shader.getReflectedData();
    uint32_t count;
    reflectedData->EnumerateDescriptorSets(&count, nullptr);
    sets.resize(count);
    reflectedData->EnumerateDescriptorSets(&count, sets.data());
    for (const auto* set : sets) {
      for (uint32_t j{}; j < set->binding_count; ++j) {
        const auto* binding = set->bindings[j];
        if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE && binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) continue;
        RenderGraph::ImageID id = RenderGraph::getImageId(binding->name);
        switch (id) {
          case RenderGraph::getImageId("albedo"): id = Tools::hash(albedoTexture.lock().get()); break;
          case RenderGraph::getImageId("normal"): id = Tools::hash(normalTexture.lock().get()); break;
          default: break;
        }
        results.emplace_back(
          id,
          RenderGraph::ImageAccess {
            .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            .access = VK_ACCESS_SHADER_READ_BIT,
            .stage = getPipelineStages(shader.getStage())
          }
        );
      }
    }
  }
  return results;
}

void Material::computeDescriptorSetRequirements(std::map<DescriptorSetRequirer*, std::vector<VkDescriptorSetLayoutBinding>>& requirements, RenderPass* renderPass, Pipeline* pipeline) {
  const std::vector shaders = {vertexShader, fragmentShader};

  // Obtain reflected descriptor set data
  std::vector<SpvReflectDescriptorSet*> sets;  // sets for all shaders
  std::vector<VkShaderStageFlags> shaderStages;
  for (uint32_t i{}; i < shaders.size(); ++i) {
    const Shader& shader = *shaders[i];
    const spv_reflect::ShaderModule* reflectedData = shader.getReflectedData();
    uint32_t count;
    reflectedData->EnumerateDescriptorSets(&count, nullptr);
    const uint32_t offset = sets.size();
    sets.resize(offset + count);
    reflectedData->EnumerateDescriptorSets(&count, sets.data() + offset);
    shaderStages.resize(offset + count, shader.getStage());
  }

  // Build requirements
  std::vector<std::vector<bool>> bindingsDefined(sets.size());
  for (uint32_t i{}; i < sets.size(); ++i) {
    const SpvReflectDescriptorSet& set = *sets.at(i);
    std::unordered_map<uint32_t, Binding>& setBinding = perSetBindings[set.set];
    // Find out which object this set belongs to
    std::vector<VkDescriptorSetLayoutBinding>* psetRequirements = nullptr;
    switch (set.set) {
      case 0: psetRequirements = &requirements[nullptr]; break;
      case 1: psetRequirements = &requirements[renderPass]; break;
      case 2: psetRequirements = &requirements[pipeline]; break;
      default: return GraphicsInstance::showError("bad set: " + std::to_string(set.set));
    }
    std::vector<VkDescriptorSetLayoutBinding>& setRequirements = *psetRequirements;
    // Build a map that will be used to merge all bindings at the same location
    std::map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
    for (const VkDescriptorSetLayoutBinding& setRequirement: setRequirements) bindings[setRequirement.binding] = setRequirement;

    for (uint32_t j{}; j < set.binding_count; ++j) {
      const SpvReflectDescriptorBinding& binding  = *set.bindings[j];
      // Check if this binding should be initialized or updated
      if (bindings.contains(binding.binding)) {
        // Update binding
        /**@todo: Warn if bindings do not match!*/
        bindings.at(binding.binding).stageFlags |= shaderStages.at(i);
      } else {
        // Initialize binding
        bindings[binding.binding] = {
          .binding = binding.binding,
          .descriptorType = static_cast<VkDescriptorType>(binding.descriptor_type),
          .descriptorCount = binding.count,
          .stageFlags = shaderStages.at(i),
          .pImmutableSamplers = VK_NULL_HANDLE
        };
        setBinding[binding.binding] = {
          .nameHash = Tools::hash(binding.name),
          .type = static_cast<VkDescriptorType>(binding.descriptor_type),
          .count = binding.count,
#if BOOTANICAL_GARDENS_ENABLE_READABLE_SHADER_VARIABLE_NAMES
          .name = binding.name
#endif
        };
      }
    }

    auto setBindings = bindings | std::ranges::views::values;
    setRequirements = {setBindings.begin(), setBindings.end()};
  }
}

std::vector<VkVertexInputBindingDescription> Material::computeVertexBindingDescriptions() const {
  const spv_reflect::ShaderModule* reflectionData = vertexShader->getReflectedData();

  uint32_t count;
  reflectionData->EnumerateInputVariables(&count, nullptr);
  std::vector<SpvReflectInterfaceVariable*> interfaceVariables(count);
  reflectionData->EnumerateInputVariables(&count, interfaceVariables.data());
  std::map<uint32_t, VkVertexInputBindingDescription> bindingDescriptions;
  for (SpvReflectInterfaceVariable* const& interfaceVariable: interfaceVariables) {
    if (interfaceVariable->built_in > 0) continue;
    uint32_t arrayValues = 1;
    for (uint32_t i = interfaceVariable->array.dims_count; i > 0;) arrayValues *= interfaceVariable->array.dims[--i];
    arrayValues = std::max(1U, arrayValues);
    const uint32_t elementSize = interfaceVariable->numeric.scalar.width / 8 * std::max(1U, interfaceVariable->numeric.vector.component_count) * std::max(1U, interfaceVariable->numeric.matrix.column_count);
    bindingDescriptions.emplace(interfaceVariable->location, VkVertexInputBindingDescription{std::numeric_limits<uint32_t>::max(), elementSize * arrayValues, Vertex::inputRates.at(interfaceVariable->name)});
  }
  count = 0;
  for (VkVertexInputBindingDescription& bindingDescription: bindingDescriptions | std::ranges::views::values)
    bindingDescription.binding = count++;
  return std::ranges::to<std::vector>(bindingDescriptions | std::ranges::views::values);
}

std::vector<VkVertexInputAttributeDescription> Material::computeVertexAttributeDescriptions() const {
  const spv_reflect::ShaderModule* reflectionData = vertexShader->getReflectedData();

  uint32_t count;
  reflectionData->EnumerateInputVariables(&count, nullptr);
  std::vector<SpvReflectInterfaceVariable*> interfaceVariables(count);
  reflectionData->EnumerateInputVariables(&count, interfaceVariables.data());
  std::map<uint32_t, VkVertexInputAttributeDescription> attributeDescriptions;
  for (SpvReflectInterfaceVariable* const& interfaceVariable: interfaceVariables) {
    if (interfaceVariable->built_in > 0) continue;
    count = 0;
    for (uint32_t i = 0; i < std::max(1U, interfaceVariable->numeric.matrix.column_count); ++i) {
      attributeDescriptions.emplace(interfaceVariable->location + i, VkVertexInputAttributeDescription{interfaceVariable->location + i, count, static_cast<VkFormat>(interfaceVariable->format), vkuFormatElementSize(static_cast<VkFormat>(interfaceVariable->format)) * i});
      count = std::numeric_limits<uint32_t>::max();
    }
  }
  count = std::numeric_limits<uint32_t>::max();
  for (VkVertexInputAttributeDescription& attributeDescription: attributeDescriptions | std::ranges::views::values) {
    if (attributeDescription.binding == 0) attributeDescription.binding = ++count;
    else attributeDescription.binding = count;
  }
  return std::ranges::to<std::vector>(attributeDescriptions | std::ranges::views::values);
}

std::vector<VkPushConstantRange> Material::computePushConstantRanges() const {
  const std::vector shaders = {vertexShader, fragmentShader};

  // Obtain reflected push constant data
  std::vector<SpvReflectBlockVariable*> pushConstantBlocks;  // sets for all shaders
  std::vector<VkShaderStageFlags> shaderStages;
  for (uint32_t i{}; i < shaders.size(); ++i) {
    const Shader& shader = *shaders[i];
    const spv_reflect::ShaderModule* reflectedData = shader.getReflectedData();
    uint32_t count;
    reflectedData->EnumeratePushConstantBlocks(&count, nullptr);
    const uint32_t offset = pushConstantBlocks.size();
    pushConstantBlocks.resize(offset + count);
    reflectedData->EnumeratePushConstantBlocks(&count, pushConstantBlocks.data() + offset);
    shaderStages.resize(offset + count, shader.getStage());
  }

  std::vector<VkPushConstantRange> pushConstantRanges(pushConstantBlocks.size());
  for (uint32_t i{}; i < pushConstantBlocks.size(); ++i) {
    pushConstantRanges[i] = {
      .stageFlags = shaderStages[i],
      .offset = pushConstantBlocks[i]->absolute_offset,
      .size = pushConstantBlocks[i]->padded_size
    };
  }
  return pushConstantRanges;
}

Material* Material::getVertexVariation(Shader* shader) const {
  const std::uint64_t id = Tools::hash(shader, fragmentShader, static_cast<std::uint64_t>(doubleSided), static_cast<std::uint64_t>(alphaMode), static_cast<std::uint64_t>(alphaCutoff), std::bit_cast<std::uint64_t>(albedoTexture.lock().get()), std::bit_cast<std::uint64_t>(normalTexture.lock().get()));
  Material* material = device->getMaterial(id, this);
  material->vertexShader = shader;
  return material;
}

Material* Material::getFragmentVariation(Shader* shader) const {
  const std::uint64_t id = Tools::hash(vertexShader, shader, static_cast<std::uint64_t>(doubleSided), static_cast<std::uint64_t>(alphaMode), static_cast<std::uint64_t>(alphaCutoff), std::bit_cast<std::uint64_t>(albedoTexture.lock().get()), std::bit_cast<std::uint64_t>(normalTexture.lock().get()));
  Material* material = device->getMaterial(id, this);
  material->fragmentShader = shader;
  return material;
}

std::uint32_t Material::getId() const { return id; }