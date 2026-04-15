#include "ShadeRenderPass.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/MeshGroup/Material.hpp"
#include "src/RenderEngine/MeshGroup/Mesh.hpp"
#include "src/RenderEngine/Pipeline/Pipeline.hpp"
#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"
#include "src/Tools/ClassName.h"

#include <volk/volk.h>
#include <vulkan/utility/vk_format_utils.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <filesystem>

ShadeRenderPass::ShadeRenderPass(RenderGraph& graph) : RenderPass(graph, OpaqueBit) {
  vertexProcessOverride = graph.device->getJSONVertexProcess("Shade Render Pass | Vertex Shader Override");
  RenderGraph::ImageParameters parameters {
#if !defined(NDEBUG)
    .name = "renderColor",
#endif
    .layers = 1,
    .mipLevels = 1,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    .sampleCount = VK_SAMPLE_COUNT_1_BIT,
    .resolution = graph.settings.renderResolution
  };
  graph.setImageParameters(RenderGraph::RenderColor, parameters);
#if !defined(NDEBUG)
  parameters.name = "MaterialIDAsDepth";
#endif
  parameters.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  parameters.format = VK_FORMAT_D32_SFLOAT;
  graph.setImageParameters(RenderGraph::getAttachmentId("MaterialIDAsDepth"), parameters);
}

void ShadeRenderPass::setup() {
  pipelines.clear();
  materialRemap.clear();

  // Collect and override objectMaterials.
  for (Material& material: graph.device->objectMaterials | std::ranges::views::values) {
    // Get the overridden material
    Material* overriddenMaterial = material.getVertexVariation(vertexProcessOverride);

    // Register the pipeline and record the override that we did in the materialRemap
    pipelines.emplace(overriddenMaterial, nullptr);
    materialRemap.emplace(&material, overriddenMaterial);
  }

  // Set up the render pass using the list of pipelines, each with a unique fragment process, and all with the same vertex process
  RenderPass::setup(pipelines | std::ranges::views::keys);

  // Notify about the access to the GBufferMaterialID image in a CommandBuffer::CopyImageToBuffer operation
  imageAccesses.emplace_back(RenderGraph::GBufferMaterialID, RenderGraph::ImageAccess{
    .layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    .access = VK_ACCESS_TRANSFER_READ_BIT,
    .stage = VK_PIPELINE_STAGE_TRANSFER_BIT
  });
}

void ShadeRenderPass::bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>&images) {
  std::vector<VkAttachmentReference> attachmentReferences(attachmentDescriptions.size());
  uint32_t i{~0U};
  for (VkAttachmentReference& attachmentReference: attachmentReferences) {
    attachmentReference.attachment = ++i;
    attachmentReference.layout     = attachmentDescriptions[i].initialLayout;
  }

  const std::array subpassDescriptions{  /**@todo: Once multiple subpasses can be properly handled, this can be fully automated by a function defined in the RenderPass class.*/
    VkSubpassDescription{
      .flags                   = 0,
      .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount    = inputAttachmentCount,
      .pInputAttachments       = inputAttachmentOffset == ~0U ? nullptr : &attachmentReferences[inputAttachmentOffset],
      .colorAttachmentCount    = colorAttachmentCount,
      .pColorAttachments       = colorAttachmentOffset == ~0U ? nullptr : &attachmentReferences[colorAttachmentOffset],
      .pResolveAttachments     = nullptr,
      .pDepthStencilAttachment = depthStencilAttachmentOffset == ~0U ? nullptr : &attachmentReferences[depthStencilAttachmentOffset],
      .preserveAttachmentCount = 0,
      .pPreserveAttachments    = nullptr
    }
  };

  const VkRenderPassCreateInfo renderPassCreateInfo{
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext           = nullptr,
    .flags           = 0,
    .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
    .pAttachments    = attachmentDescriptions.data(),
    .subpassCount    = static_cast<uint32_t>(subpassDescriptions.size()),
    .pSubpasses      = subpassDescriptions.data(),
    .dependencyCount = 0,
    .pDependencies   = nullptr
  };
  setRenderPassInfo(renderPassCreateInfo, images);
  if (const VkResult result = vkCreateRenderPass(graph.device->device, &renderPassCreateInfo, nullptr, &renderPass); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create render pass");
#if VK_EXT_debug_utils & BOOTANICAL_GARDENS_ENABLE_VULKAN_DEBUG_UTILS
  if (GraphicsInstance::extensionEnabled(Tools::hash(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))) {
    const VkDebugUtilsObjectNameInfoEXT nameInfo {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .pNext = nullptr,
      .objectType = VK_OBJECT_TYPE_RENDER_PASS,
      .objectHandle = std::bit_cast<uint64_t>(renderPass),
      .pObjectName = name.c_str()
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(graph.device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif

  for (auto& [material, pipeline]: pipelines) pipeline = graph.device->getPipeline(material, compatibility);

  framebuffer = std::make_unique<Framebuffer>(graph.device, images, renderPass);
  passData = std::make_unique<UniformBuffer<PassData>>(graph.device, "Shade Render Pass | Uniform Buffer");
  lightData = std::make_unique<UniformBuffer<LightData>>(graph.device, "Light Data");
  const auto image = graph.getImage(RenderGraph::GBufferMaterialID);
  const VkExtent3D resolution = image->getExtent();
  copyBuffer = std::make_unique<Buffer>(graph.device, "MaterialID -> Depth Copy Buffer", vkuFormatTexelBlockSize(image->getFormat()) * resolution.width * resolution.height * resolution.depth, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT);
}

void ShadeRenderPass::writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) {
  // The g-buffer texture coordinates
  uint32_t offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::GBufferTextureCoordinate)->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      }, [](void* mem){ delete static_cast<VkDescriptorImageInfo*>(mem); }))),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
  // The g-buffer normal
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::GBufferNormal)->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      }, [](void* mem){ delete static_cast<VkDescriptorImageInfo*>(mem); }))),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
  // The g-buffer tangent
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 2,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::GBufferTangent)->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      }, [](void* mem){ delete static_cast<VkDescriptorImageInfo*>(mem); }))),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
  // The g-buffer depth
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 3,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::GBufferDepth)->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      }, [](void* mem){ delete static_cast<VkDescriptorImageInfo*>(mem); }))),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
  // The light data
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 4,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImageInfo = nullptr,
      .pBufferInfo = static_cast<VkDescriptorBufferInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorBufferInfo{
        .buffer = lightData->getBuffer(),
        .offset = 0,
        .range = lightData->getSize()
      }, [](void* mem) { delete static_cast<VkDescriptorBufferInfo*>(mem); }))),
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
  // The view data
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 5,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImageInfo = nullptr,
      .pBufferInfo = static_cast<VkDescriptorBufferInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorBufferInfo{
        .buffer = passData->getBuffer(),
        .offset = 0,
        .range = passData->getSize()
      }, [](void* mem) { delete static_cast<VkDescriptorBufferInfo*>(mem); }))),
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
  // The shadow map
  offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 6,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = static_cast<VkDescriptorImageInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorImageInfo{
        .sampler     = *graph.device->getSampler(),
        .imageView   = graph.getImage(RenderGraph::ShadowMap)->getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      }, [](void* mem){ delete static_cast<VkDescriptorImageInfo*>(mem); }))),
      .pBufferInfo = nullptr,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = getDescriptorSet(i);
}

std::optional<std::pair<GraphicsDevice::ImageID, RenderGraph::ImageAccess>> ShadeRenderPass::getDepthStencilAttachmentAccess() {
  return {{RenderGraph::getAttachmentId("MaterialIDAsDepth"), {
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
    .stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
    .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
  }}};
}

void ShadeRenderPass::update() {
  passData->update({
    .inverseViewProjectionMatrix = glm::inverse(
      glm::perspectiveRH_ZO(glm::radians(60.0f), 8.0f / 6.0f, 2.0f, 1.0f) *  // Projection matrix
      glm::lookAtRH(glm::vec3(1, 1, 1), glm::vec3(0, .25, 0), glm::vec3(0, -1, 0))),  // View matrix
    .position = glm::vec3(1, 1, 1),
    .resolution = glm::vec2(graph.settings.renderResolution.width, graph.settings.renderResolution.height)
  });
  lightData->update({
    .viewProjectionMatrix = glm::orthoRH_ZO(-1.f, 1.f, -1.f, 1.f, 15.f, -15.f) *  // Projection matrix
      glm::lookAtRH(glm::vec3(-1, 10, -1), glm::vec3(0, .25, 0), glm::vec3(0, 0, -1)),
    .position = glm::vec3(-1, 10, -1)
  });
}

void ShadeRenderPass::execute(CommandBuffer& commandBuffer) {
  commandBuffer.record<CommandBuffer::CopyImageToBuffer>(graph.getImage(RenderGraph::GBufferMaterialID).get(), copyBuffer.get());
  commandBuffer.record<CommandBuffer::CopyBufferToImage>(copyBuffer.get(), graph.getImage(RenderGraph::getAttachmentId("MaterialIDAsDepth")).get());
  commandBuffer.record<CommandBuffer::BeginRenderPass>(this, clearValues);
  const uint64_t frameIndex = graph.getFrameIndex();
  for (const Pipeline* pipeline : pipelines | std::ranges::views::values) {
    commandBuffer.record<CommandBuffer::BindPipeline>(pipeline);
    commandBuffer.record<CommandBuffer::PushConstants>(pipeline->getMaterial()->id, VK_SHADER_STAGE_VERTEX_BIT);
    commandBuffer.record<CommandBuffer::BindDescriptorSets>(std::vector{getDescriptorSet(frameIndex), pipeline->getDescriptorSet(frameIndex)}, 1);
    commandBuffer.record<CommandBuffer::Draw>(3);  // No vertex buffer needs to be bound for this call because the vertex shader generates the vertex positions automatically.
  }
  commandBuffer.record<CommandBuffer::EndRenderPass>();
}

void ShadeRenderPass::bind(VkDescriptorBufferInfo& bufferInfo, Pipeline* pipeline, Material* material, const Material::Binding& info) {
  switch (info.id) {
    case Tools::hash("lightData"): {
      bufferInfo = {
        .buffer = lightData->getBuffer(),
        .offset = 0,
        .range  = lightData->getSize()
      };
      break;
    }
    case Tools::hash("viewData"): {
      bufferInfo = {
        .buffer = passData->getBuffer(),
        .offset = 0,
        .range  = passData->getSize()
      };
      break;
    }
    default: {
      RenderPass::bind(bufferInfo, pipeline, material, info);
      break;
    }
  }
}