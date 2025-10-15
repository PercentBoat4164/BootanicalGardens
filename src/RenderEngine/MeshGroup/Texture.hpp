#pragma once

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Resources/Image.hpp"

class Texture : public Image {
  std::shared_ptr<VkSampler> sampler;

public:
  [[nodiscard]] VkSampler getSampler() const;

  template <typename... Args> requires(std::constructible_from<Image, GraphicsDevice* const, const std::string&, Args&&...>) Texture(GraphicsDevice* device, const std::string& name, VkSampler* sampler, Args&&... args) : Image(device, name, args...), sampler(sampler) {}
  template <typename... Args> requires(std::constructible_from<Image, GraphicsDevice* const, const std::string&, Args&&...>) Texture(GraphicsDevice* device, const std::string& name, Args&&... args) : Image(device, name, args...), sampler(device->getSampler()) {}

  static Texture* jsonGet(GraphicsDevice* device, yyjson_val* textureJSON, CommandBuffer& commandBuffer);
};
