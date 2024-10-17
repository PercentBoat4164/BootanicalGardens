#pragma once

#include <src/RenderEngine/Image.hpp>
#include <vulkan/vulkan.h>

class Texture : public Image {
  VkSampler* sampler;

public:
  template <typename... Args> requires(std::constructible_from<Image, const GraphicsDevice&, const std::string&, Args&&...>) Texture(const GraphicsDevice& device, const std::string& name, VkSampler* sampler, Args&&... args) : Image(device, name, args...), sampler(sampler) {}
  template <typename... Args> requires(std::constructible_from<Image, const GraphicsDevice&, const std::string&, Args&&...>) Texture(const GraphicsDevice& device, const std::string& name, Args&&... args) : Image(device, name, args...), sampler(VK_NULL_HANDLE) {}
};
