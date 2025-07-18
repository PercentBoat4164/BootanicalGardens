#include "OpaqueRenderPass.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/RenderGraph.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Mesh.hpp"
#include "src/RenderEngine/Resources/Buffer.hpp"
#include "src/RenderEngine/Resources/Image.hpp"
#include "src/RenderEngine/Resources/UniformBuffer.hpp"


#include <glm/ext/matrix_clip_space.hpp>
#include <glm/matrix.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include "src/RenderEngine/GraphicsInstance.hpp"


#include <glm/gtx/transform.hpp>

#include <volk/volk.h>

OpaqueRenderPass::OpaqueRenderPass(RenderGraph& graph) : RenderPass(graph, OpaqueBit), graph(graph) {
  material = std::make_shared<Material>(
    std::make_shared<Shader>(graph.device, "../res/shaders/deferred.frag"),
    std::make_shared<Shader>(graph.device, "../res/shaders/deferred.vert")
  );
}

std::vector<std::pair<RenderGraph::AttachmentID, RenderGraph::AttachmentDeclaration>> OpaqueRenderPass::declareAttachments() {
  return {
      {RenderGraph::RenderColor, RenderGraph::AttachmentDeclaration{.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, .access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, .stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .storeOp = VK_ATTACHMENT_STORE_OP_STORE}},
      {RenderGraph::RenderDepth, RenderGraph::AttachmentDeclaration{.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, .stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .storeOp = VK_ATTACHMENT_STORE_OP_STORE}}
  };
}

void OpaqueRenderPass::bake(const std::vector<VkAttachmentDescription>& attachmentDescriptions, const std::vector<std::shared_ptr<Image>>& images) {
  const std::array attachmentReferences{
      VkAttachmentReference{
          .attachment = 0,
          .layout     = attachmentDescriptions[0].initialLayout
      },
      VkAttachmentReference{
          .attachment = 1,
          .layout     = attachmentDescriptions[1].initialLayout
      }
  };
  const std::array subpassDescriptions{
      VkSubpassDescription{
          .flags                   = 0,
          .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
          .inputAttachmentCount    = 0,
          .pInputAttachments       = nullptr,
          .colorAttachmentCount    = 1,
          .pColorAttachments       = &attachmentReferences[0],
          .pResolveAttachments     = nullptr,
          .pDepthStencilAttachment = &attachmentReferences[1],
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
  compatibility = computeRenderPassCompatibility(renderPassCreateInfo);
  if (const VkResult result = vkCreateRenderPass(graph.device->device, &renderPassCreateInfo, nullptr, &renderPass); result != VK_SUCCESS) GraphicsInstance::showError(result, "failed to create render pass");

  framebuffer = std::make_shared<Framebuffer>(graph.device, images, renderPass);
  for (const std::shared_ptr<Mesh>& mesh: meshes) {
    const std::shared_ptr<Material> material = mesh->getMaterial();
    pipelines[material]                      = std::make_shared<Pipeline>(graph.device, material);
  }

  uniformBuffer = std::make_shared<UniformBuffer<PassData>>(graph.device, "uniform buffer");
}

void OpaqueRenderPass::writeDescriptorSets(std::vector<void*>& miscMemoryPool, std::vector<VkWriteDescriptorSet>& writes) {
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

void OpaqueRenderPass::update(const RenderGraph& graph) {
  const glm::mat4x4 projectionMatrix = glm::perspectiveZO(glm::radians(60.0f), 8.0f / 6.0f, 0.001f, 1000.0f);
  const glm::mat4x4 viewMatrix       = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0, .25, 0), glm::vec3(0, -1, 0));
  const PassData passData {projectionMatrix * viewMatrix};
  uniformBuffer->update(passData);
}

void OpaqueRenderPass::execute(CommandBuffer& commandBuffer) {
  commandBuffer.record<CommandBuffer::BeginRenderPass>(shared_from_this());
  for (const std::shared_ptr<Mesh>& mesh : meshes) {
    commandBuffer.record<CommandBuffer::BindVertexBuffers>(std::vector<std::tuple<std::shared_ptr<Buffer>, const VkDeviceSize>>{{mesh->getVertexBuffer(), 0}});
    const bool meshIsIndexed = mesh->getIndexBuffer() != nullptr;
    if (meshIsIndexed) commandBuffer.record<CommandBuffer::BindIndexBuffers>(mesh->getIndexBuffer());
    const std::shared_ptr<Pipeline>& pipeline = pipelines.at(mesh->getMaterial());
    commandBuffer.record<CommandBuffer::BindPipeline>(pipeline);
    const uint64_t frameIndex = graph.getFrameIndex();
    commandBuffer.record<CommandBuffer::BindDescriptorSets>(std::vector{*graph.getPerFrameData(frameIndex).descriptorSet, *getDescriptorSet(frameIndex), *pipeline->getDescriptorSet(frameIndex)});
    if (meshIsIndexed) commandBuffer.record<CommandBuffer::DrawIndexed>();
    else commandBuffer.record<CommandBuffer::Draw>();
  }
  commandBuffer.record<CommandBuffer::EndRenderPass>();
}