#include "ShadowRenderPass.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Mesh.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"

#include <volk/volk.h>

ShadowRenderPass::ShadowRenderPass(RenderGraph& graph) : RenderPass(graph, OpaqueBit) {
  material = std::make_shared<Material>(
    std::make_shared<Shader>(graph.device, std::filesystem::canonical("../res/shaders/shadow.vert")),
    std::make_shared<Shader>(graph.device, std::filesystem::canonical("../res/shaders/shadow.frag"))
  );
}

std::vector<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> ShadowRenderPass::declareAccesses() {
  std::array materials{material};
  setup(materials);
  return imageAccesses;
}

void ShadowRenderPass::bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) {
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
  const VkRenderPassCreateInfo renderPassCreateInfo {
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
      .pObjectName = "Shadow Render Pass"
    };
    if (const VkResult result = vkSetDebugUtilsObjectNameEXT(graph.device->device, &nameInfo); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to set debug utils object name");
  }
#endif

  framebuffer = std::make_shared<Framebuffer>(graph.device, images, renderPass);
  pipelines[material] = std::make_shared<Pipeline>(graph.device, material);

  uniformBuffer = std::make_shared<UniformBuffer<PassData>>(graph.device, "Shadow Pass | Uniform Buffer");
}

void ShadowRenderPass::writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes, const RenderGraph&graph) {
  const auto bufferInfo = static_cast<VkDescriptorBufferInfo*>(miscMemoryPool.emplace_back(new VkDescriptorBufferInfo{
     .buffer = uniformBuffer->getBuffer(),
     .offset = 0,
     .range = uniformBuffer->getSize()
  }));
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

std::optional<std::pair<RenderGraph::ImageID, RenderGraph::ImageAccess>> ShadowRenderPass::getDepthStencilAttachmentAccess() {
  return {{RenderGraph::getImageId(RenderGraph::ShadowDepth), {
    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
  }}};
}

void ShadowRenderPass::update() {
  const glm::mat4x4 projectionMatrix = glm::orthoRH_ZO(-1.f, 1.f, -1.f, 1.f, -15.f, 15.f);
  const glm::mat4x4 viewMatrix       = glm::lookAtRH(glm::vec3(1, 10, 1), glm::vec3(0, .25, 0), glm::vec3(0, 0, -1));
  const PassData passData {
    .light_ViewProjectionMatrix = projectionMatrix * viewMatrix,
  };
  uniformBuffer->update(passData);
}

void ShadowRenderPass::execute(CommandBuffer& commandBuffer) {
  commandBuffer.record<CommandBuffer::BeginRenderPass>(shared_from_this(), VkRect2D{}, std::vector<VkClearValue>{{.depthStencil = {.depth = 1.0F, .stencil = 0}}});
  commandBuffer.record<CommandBuffer::BindPipeline>(pipelines[material]);
  commandBuffer.record<CommandBuffer::BindDescriptorSets>(std::vector{*getDescriptorSet(graph.getFrameIndex())}, 1);
  for (const std::shared_ptr<Mesh>& mesh : graph.device->meshes) {
    if (!(mesh->isOpaque() && meshFilter & OpaqueBit) && !(mesh->isTransparent() && meshFilter & TransparentBit))
      continue;
    commandBuffer.record<CommandBuffer::BindVertexBuffers>(std::vector<std::tuple<std::shared_ptr<Buffer>, const VkDeviceSize>>{{mesh->getVertexBuffer(), 0}});
    if (mesh->getIndexBuffer() != nullptr) {
      commandBuffer.record<CommandBuffer::BindIndexBuffers>(mesh->getIndexBuffer());
      commandBuffer.record<CommandBuffer::DrawIndexed>();
    } else commandBuffer.record<CommandBuffer::Draw>();
  }
  commandBuffer.record<CommandBuffer::EndRenderPass>();
}