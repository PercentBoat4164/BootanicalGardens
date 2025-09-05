#include "Material.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/Renderable/Texture.hpp"
#include "src/RenderEngine/Resources/StagingBuffer.hpp"

#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <ranges>
#include <set>
#include <volk/volk.h>

const std::byte* handleDataSource(const fastgltf::Asset& asset, const fastgltf::DataSource& source, size_t* size) {
  return std::visit(fastgltf::visitor {
        [&size](const auto& arg) -> const std::byte* {
          return reinterpret_cast<std::byte*>(*size = 0);
        },
        [&asset, &size](const fastgltf::sources::BufferView& view){
          auto& bufferView = asset.bufferViews[view.bufferViewIndex];
          *size = bufferView.byteLength;
          auto& buffer = asset.buffers[bufferView.bufferIndex];
          std::size_t dummy;
          return handleDataSource(asset, buffer.data, &dummy) + bufferView.byteOffset;
        },
        [&size](const fastgltf::sources::Array& vector){
          *size = vector.bytes.size();
          return vector.bytes.data();
        }
      }, source);
}

template<typename T>
requires std::derived_from<T, fastgltf::TextureInfo>
void loadTexture(GraphicsDevice* const device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, std::shared_ptr<Texture>* texture, const fastgltf::Optional<T>* textureInfo) {
  if (!textureInfo->has_value()) return;
  const fastgltf::Optional<std::size_t>& imageIndex = asset.textures[textureInfo->value().textureIndex].imageIndex;
  if (!imageIndex.has_value()) return;
  const fastgltf::Image& image = asset.images[imageIndex.value()];
  std::size_t size;
  const auto* bytes    = handleDataSource(asset, image.data, &size);
  SDL_IOStream* io     = SDL_IOFromConstMem(bytes, static_cast<int>(size));
  SDL_Surface* surface = IMG_Load_IO(io, true);
  const std::vector textureBytes(static_cast<std::byte*>(surface->pixels), static_cast<std::byte*>(surface->pixels) + surface->w * surface->h * SDL_BYTESPERPIXEL(surface->format));
  auto buffer = std::make_shared<StagingBuffer>(device, std::string{image.name + " upload buffer"}.c_str(), textureBytes);
  VkFormat format;
  switch (surface->format) {
    case SDL_PIXELFORMAT_ARGB4444: format = VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT; break;
    case SDL_PIXELFORMAT_RGBA4444: format = VK_FORMAT_R4G4B4A4_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_ABGR4444: format = VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT; break;
    case SDL_PIXELFORMAT_BGRA4444: format = VK_FORMAT_B4G4R4A4_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_ARGB1555: format = VK_FORMAT_A1R5G5B5_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_RGBA5551: format = VK_FORMAT_R5G5B5A1_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_ABGR1555: format = VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR; break;
    case SDL_PIXELFORMAT_BGRA5551: format = VK_FORMAT_B5G5R5A1_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_RGB565: format = VK_FORMAT_R5G6B5_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_BGR565: format = VK_FORMAT_B5G6R5_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_RGB24: format = VK_FORMAT_R8G8B8_SRGB; break;
    case SDL_PIXELFORMAT_BGR24: format = VK_FORMAT_B8G8R8_SRGB; break;
    case SDL_PIXELFORMAT_RGBA32: format = VK_FORMAT_R8G8B8A8_SRGB; break;
    case SDL_PIXELFORMAT_ABGR32: format = VK_FORMAT_A8B8G8R8_SRGB_PACK32; break;
    case SDL_PIXELFORMAT_BGRA32: format = VK_FORMAT_B8G8R8A8_SRGB; break;
    case SDL_PIXELFORMAT_RGBX32: format = VK_FORMAT_R8G8B8A8_SRGB; break;
    case SDL_PIXELFORMAT_XBGR32: format = VK_FORMAT_A8B8G8R8_SRGB_PACK32; break;
    case SDL_PIXELFORMAT_BGRX32: format = VK_FORMAT_B8G8R8A8_SRGB; break;
    case SDL_PIXELFORMAT_ARGB2101010: format = VK_FORMAT_A2R10G10B10_UNORM_PACK32; break;
    default: format = VK_FORMAT_UNDEFINED;
  }
  *texture = std::make_shared<Texture>(device, std::string{image.name}, format, VkExtent3D{static_cast<uint32_t>(surface->w), static_cast<uint32_t>(surface->h), 1U}, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  std::vector<VkBufferImageCopy> regions{
    {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = VkImageSubresourceLayers{
        .aspectMask     = (*texture)->getAspect(),
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = (*texture)->getLayerCount(),
      },
      .imageOffset = {},
      .imageExtent = (*texture)->getExtent()
    }
  };
  commandBuffer.record<CommandBuffer::CopyBufferToImage>(buffer, *texture, regions);
  SDL_DestroySurface(surface);
}

Material::Material(const std::shared_ptr<const Shader>& vertexShader, const std::shared_ptr<const Shader>& fragmentShader) : vertexShader(vertexShader), fragmentShader(fragmentShader) {}
Material::Material(GraphicsDevice* const device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Material& material) : name(material.name),
                                                                                                                                                   doubleSided(material.doubleSided),
                                                                                                                                                   alphaMode(material.alphaMode),
                                                                                                                                                   alphaCutoff(material.alphaCutoff),
                                                                                                                                                   unlit(material.unlit),
                                                                                                                                                   ior(material.ior),
                                                                                                                                                   dispersion(material.dispersion),
                                                                                                                                                   albedoFactor(material.pbrData.baseColorFactor.x(), material.pbrData.baseColorFactor.y(), material.pbrData.baseColorFactor.z(), material.pbrData.baseColorFactor.w()),
                                                                                                                                                   normalFactor(material.normalTexture.has_value() ? material.normalTexture->scale : 1.f),
                                                                                                                                                   occlusionFactor(material.occlusionTexture.has_value() ? material.occlusionTexture->strength : 1.f),
                                                                                                                                                   emissiveFactor(material.emissiveFactor.x(), material.emissiveFactor.y(), material.emissiveFactor.z()),
                                                                                                                                                   anisotropyFactor(material.anisotropy ? material.anisotropy->anisotropyStrength : 0.f),
                                                                                                                                                   anisotropyRotation(material.anisotropy ? material.anisotropy->anisotropyRotation : 0.f),
                                                                                                                                                   metallicFactor(material.pbrData.metallicFactor),
                                                                                                                                                   roughnessFactor(material.pbrData.roughnessFactor) {
  /**@todo: This could be immensely sped up using multi-threading (most lucrative option because most time is spent on the CPU in loadTexture calls) and by using a dedicated transfer queue for these operations.
   *    I refrained from doing these things because I do not know what the multi-threading of this game is going to look like yet, and I am not ready to make that decision (especially without Ethan).
   */
  loadTexture<decltype(material.pbrData.baseColorTexture)::value_type>(device, commandBuffer, asset, &albedoTexture, &material.pbrData.baseColorTexture);
  loadTexture<decltype(material.normalTexture)::value_type>(device, commandBuffer, asset, &normalTexture, &material.normalTexture);
  // loadTexture<decltype(material.occlusionTexture)::value_type>(device, commandBuffer, asset, &occlusionTexture, &material.occlusionTexture);
  // loadTexture<decltype(material.emissiveTexture)::value_type>(device, commandBuffer, asset, &emissiveTexture, &material.emissiveTexture);
  // if (material.anisotropy) loadTexture<decltype(material.anisotropy->anisotropyTexture)::value_type>(device, commandBuffer, asset, &anisotropyTexture, &material.anisotropy->anisotropyTexture);
  // loadTexture<decltype(material.pbrData.metallicRoughnessTexture)::value_type>(device, commandBuffer, asset, &metallicRoughnessTexture, &material.pbrData.metallicRoughnessTexture);
}

bool Material::isDoubleSided() const { return doubleSided; }
fastgltf::AlphaMode Material::getAlphaMode() const { return alphaMode; }
float Material::getAlphaCutoff() const { return alphaCutoff; }
bool Material::isUnlit() const { return unlit; }
float Material::getIor() const { return ior; }
float Material::getDispersion() const { return dispersion; }
std::shared_ptr<Texture> Material::getAlbedoTexture() const { return albedoTexture; }
std::string Material::getAlbedoTextureName() const { if (albedoTexture) return name + " | Albedo Texture"; return std::string(RenderGraph::VoidImage); }
glm::vec4 Material::getAlbedoFactor() const { return albedoFactor; }
std::shared_ptr<Texture> Material::getNormalTexture() const { return normalTexture; }
std::string Material::getNormalTextureName() const { if (albedoTexture) return name + " | Normal Texture"; return std::string(RenderGraph::VoidImage); }
float Material::getNormalFactor() const { return normalFactor; }
std::shared_ptr<Texture> Material::getOcclusionTexture() const { return occlusionTexture; }
float Material::getOcclusionFactor() const { return occlusionFactor; }
std::shared_ptr<Texture> Material::getEmissiveTexture() const { return emissiveTexture; }
glm::vec3 Material::getEmissiveFactor() const { return emissiveFactor; }
std::shared_ptr<Texture> Material::getAnisotropyTexture() const { return anisotropyTexture; }
float Material::getAnisotropyFactor() const { return anisotropyFactor; }
float Material::getAnisotropyRotation() const { return anisotropyRotation; }
std::shared_ptr<Texture> Material::getMetallicRoughnessTexture() const { return metallicRoughnessTexture; }
float Material::getMetallicFactor() const { return metallicFactor; }
float Material::getRoughnessRotation() const { return roughnessFactor; }
void Material::setVertexShader(const std::shared_ptr<const Shader>& shader) { vertexShader = shader; }
std::shared_ptr<const Shader> Material::getVertexShader() const { return vertexShader; }
void Material::setFragmentShader(const std::shared_ptr<const Shader>& shader) { fragmentShader = shader; }
std::shared_ptr<const Shader> Material::getFragmentShader() const { return fragmentShader; }
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
          case RenderGraph::getImageId("albedo"): id = RenderGraph::getImageId(getAlbedoTextureName()); break;
          case RenderGraph::getImageId("normal"): id = RenderGraph::getImageId(getNormalTextureName()); break;
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

void Material::computeDescriptorSetRequirements(std::map<std::shared_ptr<DescriptorSetRequirer>, std::vector<VkDescriptorSetLayoutBinding>>& requirements, const std::shared_ptr<RenderPass>& renderPass, const std::shared_ptr<Pipeline>& pipeline) {
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
      case 2: psetRequirements = &requirements[std::reinterpret_pointer_cast<DescriptorSetRequirer>(pipeline)]; break;
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
#if BOOTANICAL_GARDENS_ENABLE_READABLE_SHADER_VARIABLE_NAMES
          .name = binding.name,
#endif
          .nameHash = Tools::hash(binding.name),
          .type = static_cast<VkDescriptorType>(binding.descriptor_type),
          .count = binding.count
        };
      }
    }

    auto setBindings = bindings | std::ranges::views::values;
    setRequirements = {setBindings.begin(), setBindings.end()};
  }
}