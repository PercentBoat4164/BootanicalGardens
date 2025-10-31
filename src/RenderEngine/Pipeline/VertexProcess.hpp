#pragma once

#include <vulkan/vulkan_core.h>

class GraphicsDevice;
class Shader;

class yyjson_val;

struct VertexProcess {
  Shader* shader = nullptr;

  VkPrimitiveTopology topology    = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  VkBool32 primitiveRestartEnable = VK_FALSE;
  VkCullModeFlags cullMode        = VK_CULL_MODE_BACK_BIT;
  VkFrontFace frontFace           = VK_FRONT_FACE_CLOCKWISE;

  static VertexProcess jsonGet(GraphicsDevice* device, yyjson_val* jsonData);
};