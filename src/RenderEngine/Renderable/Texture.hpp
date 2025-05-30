#pragma once

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Resources/Image.hpp"

#include <volk/volk.h>

class Texture : public Image {
  std::shared_ptr<VkSampler> sampler;

public:
  [[nodiscard]] VkSampler getSampler() const;

  template <typename... Args> requires(std::constructible_from<Image, const std::shared_ptr<GraphicsDevice>&, const std::string&, Args&&...>) Texture(const std::shared_ptr<GraphicsDevice>& device, const std::string& name, VkSampler* sampler, Args&&... args) : Image(device, name, args...), sampler(sampler) {}
  template <typename... Args> requires(std::constructible_from<Image, const std::shared_ptr<GraphicsDevice>&, const std::string&, Args&&...>) Texture(const std::shared_ptr<GraphicsDevice>& device, const std::string& name, Args&&... args) : Image(device, name, args...), sampler(device->getSampler()) {}
};
