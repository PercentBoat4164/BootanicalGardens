#include "VertexProcess.hpp"

#include "src/RenderEngine/GraphicsDevice.hpp"

#include <magic_enum/magic_enum.hpp>

VertexProcess VertexProcess::jsonGet(GraphicsDevice* device, yyjson_val* jsonData) {
  VertexProcess vertexProcess;
  std::uint64_t uintValue;
  const char* enumValue;
  bool boolValue;
  yyjson_ptr_get_uint(jsonData, "/shader", &uintValue);
  vertexProcess.shader = device->getJSONShader(uintValue);
  yyjson_ptr_get_str(jsonData, "/topology", &enumValue);
  vertexProcess.topology = magic_enum::enum_cast<VkPrimitiveTopology>(enumValue).value_or(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  yyjson_ptr_get_bool(jsonData, "/primitiveRestartEnable", &boolValue);
  vertexProcess.primitiveRestartEnable = boolValue;
  yyjson_ptr_get_str(jsonData, "/cullMode", &enumValue);
  vertexProcess.cullMode = magic_enum::enum_cast<VkCullModeFlagBits>(enumValue).value_or(VK_CULL_MODE_BACK_BIT);
  yyjson_ptr_get_str(jsonData, "/frontFace", &enumValue);
  vertexProcess.frontFace = magic_enum::enum_cast<VkFrontFace>(enumValue).value_or(VK_FRONT_FACE_CLOCKWISE);
  return vertexProcess;
}