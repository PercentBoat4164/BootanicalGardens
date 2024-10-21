#pragma once

#include "Vertex.hpp"

#include <fastgltf/core.hpp>

#include <vulkan/vulkan.h>

#include <vector>

class Buffer;
class CommandBuffer;
class GraphicsDevice;
class Material;

class Mesh {
  VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_MAX_ENUM};
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::shared_ptr<Material> material{nullptr};
  std::shared_ptr<Buffer> vertexBuffer{nullptr};
  std::shared_ptr<Buffer> indexBuffer{nullptr};

public:
  Mesh(const GraphicsDevice& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Primitive& primitive);
};
