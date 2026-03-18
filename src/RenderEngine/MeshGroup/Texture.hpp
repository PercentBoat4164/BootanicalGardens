#pragma once

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/Resources/Image.hpp"

class Texture : public Image {
  VkSampler* sampler;

public:
  [[nodiscard]] VkSampler getSampler() const override;

  template <typename... Args> requires(std::constructible_from<Image, GraphicsDevice* const,
#if !defined(NDEBUG)
    const std::string&,
#endif
    Args&&...>) Texture(GraphicsDevice* device,
#if !defined(NDEBUG)
    const std::string& name,
#endif
    VkSampler* sampler, Args&&... args) : Image(device,
#if !defined(NDEBUG)
      name,
#endif
      args...), sampler(sampler) {}

  template <typename... Args> requires(std::constructible_from<Image, GraphicsDevice* const,
#if !defined(NDEBUG)
    const std::string&,
#endif
    Args&&...>) Texture(GraphicsDevice* device,
#if !defined(NDEBUG)
      const std::string& name,
#endif
      Args&&... args) : Image(device,
#if !defined(NDEBUG)
        name,
#endif
        args...), sampler(device->getSampler()) {}

  static std::unique_ptr<Texture> jsonGet(GraphicsDevice* device, yyjson_val* textureJSON, CommandBuffer& commandBuffer);
};
