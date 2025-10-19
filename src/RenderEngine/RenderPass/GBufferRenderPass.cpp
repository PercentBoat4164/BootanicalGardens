#include "GBufferRenderPass.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/RenderGraph.hpp"
#include "src/RenderEngine/MeshGroup/Material.hpp"
#include "src/RenderEngine/MeshGroup/Mesh.hpp"
#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <volk/volk.h>

GBufferRenderPass::GBufferRenderPass(RenderGraph& graph) : RenderPass(graph, OpaqueBit) {
  fragmentShaderOverride = graph.device->getJSONShader("Geometry Buffer Render Pass | Fragment Shader Override");
}

void GBufferRenderPass::setup() {
  pipelines.clear();
  materialRemap.clear();
  for (const Mesh& mesh: graph.device->meshes | std::ranges::views::values) {
    for (Material* material : mesh.instances | std::ranges::views::keys) {
      Material* overriddenMaterial = material->getFragmentVariation(fragmentShaderOverride);
      pipelines.emplace(overriddenMaterial, nullptr);
      materialRemap.emplace(material, overriddenMaterial);
    }
  }
  RenderPass::setup(pipelines | std::ranges::views::keys);
}

void GBufferRenderPass::bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<const Image*>&images) {
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
      .objectHandle = reinterpret_cast<uint64_t>(renderPass),
      .pObjectName = "G-Buffer Render Pass"
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(graph.device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif

  for (auto& [material, pipeline]: pipelines) pipeline = graph.device->getPipeline(material, compatibility);

  framebuffer = std::make_unique<Framebuffer>(graph.device, images, renderPass);
  uniformBuffer = std::make_unique<UniformBuffer<PassData>>(graph.device, "G-Buffer Render Pass | Uniform Buffer");
}

void GBufferRenderPass::writeDescriptorSets(std::deque<std::tuple<void*, std::function<void(void*)>>>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph&graph) {
  const auto bufferInfo = static_cast<VkDescriptorBufferInfo*>(std::get<0>(miscMemoryPool.emplace_back(new VkDescriptorBufferInfo{
       .buffer = uniformBuffer->getBuffer(),
       .offset = 0,
       .range = uniformBuffer->getSize()
  }, [](void* mem) { delete static_cast<VkDescriptorBufferInfo*>(mem); })));
  const uint32_t offset = writes.size();
  writes.resize(offset + descriptorSets.size(), {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = nullptr,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pImageInfo = nullptr,
      .pBufferInfo = bufferInfo,
      .pTexelBufferView = nullptr
  });
  for (uint64_t i{}; i < descriptorSets.size(); ++i) writes[offset + i].dstSet = *getDescriptorSet(i);
}

std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> GBufferRenderPass::getDepthStencilAttachmentAccess() {
  return {{RenderGraph::getImageId(RenderGraph::GBufferDepth), {
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
  }}};
}

void GBufferRenderPass::update() {
  const glm::mat4x4 projectionMatrix = glm::perspectiveRH_ZO(glm::radians(60.0f), 8.0f / 6.0f, 1.0f, 2.0f);
  const glm::mat4x4 viewMatrix       = glm::lookAtRH(glm::vec3(1, 1, 1), glm::vec3(0, .25, 0), glm::vec3(0, -1, 0));
  const PassData passData {
    .view_ViewProjectionMatrix = projectionMatrix * viewMatrix
  };
  uniformBuffer->update(passData);
}

void GBufferRenderPass::execute(CommandBuffer& commandBuffer) {
  const uint64_t frameIndex = graph.getFrameIndex();
  commandBuffer.record<CommandBuffer::BeginRenderPass>(this, clearValues);
  for (const Mesh& mesh : graph.device->meshes | std::ranges::views::values) {
    commandBuffer.record<CommandBuffer::BindVertexBuffers>(std::array{mesh.positionsVertexBuffer.get(), mesh.textureCoordinatesVertexBuffer.get(), mesh.normalsVertexBuffer.get(), mesh.tangentsVertexBuffer.get()});
    commandBuffer.record<CommandBuffer::BindIndexBuffer>(mesh.indexBuffer.get());
    for (auto& [material, instanceData]: mesh.instances) {
      Pipeline* pipeline = pipelines.at(materialRemap.at(material));
      commandBuffer.record<CommandBuffer::BindPipeline>(pipeline);
      commandBuffer.record<CommandBuffer::BindDescriptorSets>(std::array{*getDescriptorSet(graph.getFrameIndex()), *pipeline->getDescriptorSet(frameIndex)}, 1);
      commandBuffer.record<CommandBuffer::BindVertexBuffers>(std::array{instanceData.modelInstanceBuffer.get(), instanceData.materialInstanceBuffer.get()}, 4);
      commandBuffer.record<CommandBuffer::DrawIndexed>(instanceData.perInstanceData.size());
    }
  }
  commandBuffer.record<CommandBuffer::EndRenderPass>();
}