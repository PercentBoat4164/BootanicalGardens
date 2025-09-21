#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include <glm/fwd.hpp>
#include <glm/gtc/vec1.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace detail {
  template<typename> static consteval VkFormat formatOf() { return VK_FORMAT_UNDEFINED; }
  template<> consteval VkFormat formatOf<glm::u8vec1>() { return VK_FORMAT_R8_UINT; }
  template<> consteval VkFormat formatOf<glm::u8vec2>() { return VK_FORMAT_R8G8_UINT; }
  template<> consteval VkFormat formatOf<glm::u8vec3>() { return VK_FORMAT_R8G8B8_UINT; }
  template<> consteval VkFormat formatOf<glm::u8vec4>() { return VK_FORMAT_R8G8B8A8_UINT; }
  template<> consteval VkFormat formatOf<glm::u16vec1>() { return VK_FORMAT_R16_UINT; }
  template<> consteval VkFormat formatOf<glm::u16vec2>() { return VK_FORMAT_R16G16_UINT; }
  template<> consteval VkFormat formatOf<glm::u16vec3>() { return VK_FORMAT_R16G16B16_UINT; }
  template<> consteval VkFormat formatOf<glm::u16vec4>() { return VK_FORMAT_R16G16B16A16_UINT; }
  template<> consteval VkFormat formatOf<glm::u32vec1>() { return VK_FORMAT_R32_UINT; }
  template<> consteval VkFormat formatOf<glm::u32vec2>() { return VK_FORMAT_R32G32_UINT; }
  template<> consteval VkFormat formatOf<glm::u32vec3>() { return VK_FORMAT_R32G32B32_UINT; }
  template<> consteval VkFormat formatOf<glm::u32vec4>() { return VK_FORMAT_R32G32B32A32_UINT; }
  template<> consteval VkFormat formatOf<glm::u64vec1>() { return VK_FORMAT_R64_UINT; }
  template<> consteval VkFormat formatOf<glm::u64vec2>() { return VK_FORMAT_R64G64_UINT; }
  template<> consteval VkFormat formatOf<glm::u64vec3>() { return VK_FORMAT_R64G64B64_UINT; }
  template<> consteval VkFormat formatOf<glm::u64vec4>() { return VK_FORMAT_R64G64B64A64_UINT; }
  template<> consteval VkFormat formatOf<glm::i8vec1>() { return VK_FORMAT_R8_SINT; }
  template<> consteval VkFormat formatOf<glm::i8vec2>() { return VK_FORMAT_R8G8_SINT; }
  template<> consteval VkFormat formatOf<glm::i8vec3>() { return VK_FORMAT_R8G8B8_SINT; }
  template<> consteval VkFormat formatOf<glm::i8vec4>() { return VK_FORMAT_R8G8B8A8_SINT; }
  template<> consteval VkFormat formatOf<glm::i16vec1>() { return VK_FORMAT_R16_SINT; }
  template<> consteval VkFormat formatOf<glm::i16vec2>() { return VK_FORMAT_R16G16_SINT; }
  template<> consteval VkFormat formatOf<glm::i16vec3>() { return VK_FORMAT_R16G16B16_SINT; }
  template<> consteval VkFormat formatOf<glm::i16vec4>() { return VK_FORMAT_R16G16B16A16_SINT; }
  template<> consteval VkFormat formatOf<glm::i32vec1>() { return VK_FORMAT_R32_SINT; }
  template<> consteval VkFormat formatOf<glm::i32vec2>() { return VK_FORMAT_R32G32_SINT; }
  template<> consteval VkFormat formatOf<glm::i32vec3>() { return VK_FORMAT_R32G32B32_SINT; }
  template<> consteval VkFormat formatOf<glm::i32vec4>() { return VK_FORMAT_R32G32B32A32_SINT; }
  template<> consteval VkFormat formatOf<glm::i64vec1>() { return VK_FORMAT_R64_SINT; }
  template<> consteval VkFormat formatOf<glm::i64vec2>() { return VK_FORMAT_R64G64_SINT; }
  template<> consteval VkFormat formatOf<glm::i64vec3>() { return VK_FORMAT_R64G64B64_SINT; }
  template<> consteval VkFormat formatOf<glm::i64vec4>() { return VK_FORMAT_R64G64B64A64_SINT; }
  template<> consteval VkFormat formatOf<glm::f32vec1>() { return VK_FORMAT_R32_SFLOAT; }
  template<> consteval VkFormat formatOf<glm::f32vec2>() { return VK_FORMAT_R32G32_SFLOAT; }
  template<> consteval VkFormat formatOf<glm::f32vec3>() { return VK_FORMAT_R32G32B32_SFLOAT; }
  template<> consteval VkFormat formatOf<glm::f32vec4>() { return VK_FORMAT_R32G32B32A32_SFLOAT; }
  template<> consteval VkFormat formatOf<glm::f64vec1>() { return VK_FORMAT_R64_SFLOAT; }
  template<> consteval VkFormat formatOf<glm::f64vec2>() { return VK_FORMAT_R64G64_SFLOAT; }
  template<> consteval VkFormat formatOf<glm::f64vec3>() { return VK_FORMAT_R64G64B64_SFLOAT; }
  template<> consteval VkFormat formatOf<glm::f64vec4>() { return VK_FORMAT_R64G64B64A64_SFLOAT; }
}

class Vertex {
public:
  glm::vec3 position;
  glm::vec2 textureCoordinates;
  glm::vec3 normal;
  glm::vec4 tangent;

  template<typename T> static consteval VkFormat formatOf() { return detail::formatOf<T>(); }

  static consteval VkVertexInputBindingDescription getBindingDescription() {
    return {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
  }

  static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
    return {
        {
          .location = 0,
          .binding  = 0,
          .format   = formatOf<decltype(position)>(),
          .offset   = offsetof(Vertex, position)
        }, {
          .location = 1,
          .binding  = 0,
          .format   = formatOf<decltype(textureCoordinates)>(),
          .offset   = offsetof(Vertex, textureCoordinates)
        }, {
          .location = 2,
          .binding  = 0,
          .format   = formatOf<decltype(normal)>(),
          .offset   = offsetof(Vertex, normal)
        }, {
          .location = 3,
          .binding  = 0,
          .format   = formatOf<decltype(tangent)>(),
          .offset   = offsetof(Vertex, tangent)
        }
    };
  }
};