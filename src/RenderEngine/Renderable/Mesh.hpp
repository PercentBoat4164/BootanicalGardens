#pragma once

#include <fastgltf/core.hpp>

#include <vulkan/vulkan.h>

#include <vector>

class Buffer;
class GraphicsDevice;
class Material;
class Vertex;

class Mesh {
  VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_MAX_ENUM};
  std::vector<Vertex> vertices{};
  std::vector<uint32_t> indices{};
  Material* material{nullptr};
  Buffer* vertexBuffer{nullptr};
  Buffer* indexBuffer{nullptr};

public:
  Mesh(const GraphicsDevice& device, const fastgltf::Asset& asset, const fastgltf::Primitive& primitive);
  ~Mesh();
};
