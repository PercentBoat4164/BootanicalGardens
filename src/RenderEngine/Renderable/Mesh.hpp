#pragma once

#include "Vertex.hpp"
#include "src/RenderEngine/RenderGraph.hpp"

#include <fastgltf/core.hpp>

#include <vulkan/vulkan.h>

#include <vector>

class Buffer;
class CommandBuffer;
class GraphicsDevice;
class Material;
template<typename T> class UniformBuffer;

class Mesh : public DescriptorSetRequirer {
  VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_MAX_ENUM};
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::shared_ptr<Material> material{nullptr};
  std::shared_ptr<Buffer> vertexBuffer{nullptr};
  std::shared_ptr<Buffer> indexBuffer{nullptr};
  std::shared_ptr<UniformBuffer<glm::mat4>> uniformBuffer;

public:
  Mesh(const std::shared_ptr<GraphicsDevice>& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Primitive& primitive);

  void update(const RenderGraph& graph) const;

  [[nodiscard]] bool isOpaque() const;
  [[nodiscard]] bool isTransparent() const;
  [[nodiscard]] std::shared_ptr<Material> getMaterial() const;
  [[nodiscard]] std::shared_ptr<Buffer> getVertexBuffer() const;
  [[nodiscard]] std::shared_ptr<Buffer> getIndexBuffer() const;
};
