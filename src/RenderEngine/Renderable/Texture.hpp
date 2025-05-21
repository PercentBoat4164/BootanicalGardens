#pragma once

#include <src/RenderEngine/Image.hpp>
#include <vulkan/vulkan.h>

class Texture : public Image {
  VkSampler* sampler;

public:
  [[nodiscard]] VkSampler getSampler() const;

  template <typename... Args> requires(std::constructible_from<Image, const std::shared_ptr<GraphicsDevice>&, const std::string&, Args&&...>) Texture(const std::shared_ptr<GraphicsDevice>& device, const std::string& name, VkSampler* sampler, Args&&... args) : Image(device, name, args...), sampler(sampler) {}
  template <typename... Args> requires(std::constructible_from<Image, const std::shared_ptr<GraphicsDevice>&, const std::string&, Args&&...>) Texture(const std::shared_ptr<GraphicsDevice>& device, const std::string& name, Args&&... args) : Image(device, name, args...), sampler(VK_NULL_HANDLE) {}
};
