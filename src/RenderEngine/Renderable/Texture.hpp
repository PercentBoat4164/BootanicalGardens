#pragma once

#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Resources/Image.hpp"

#include <volk/volk.h>

class Texture : public Image {
  VkSampler* sampler;

public:
  [[nodiscard]] VkSampler getSampler() const;

  template <typename... Args> requires(std::constructible_from<Image, std::shared_ptr<GraphicsDevice>, const std::string&, Args&&...>) Texture(std::shared_ptr<GraphicsDevice> device, const std::string& name, VkSampler* sampler, Args&&... args) : Image(device, name, args...), sampler(sampler) {}
  /**@todo: This constructor leaks the memory of the VkSampler. Write a system to track and deduplicate VkSamplers.*/
  template <typename... Args> requires(std::constructible_from<Image, std::shared_ptr<GraphicsDevice>, const std::string&, Args&&...>) Texture(std::shared_ptr<GraphicsDevice> device, const std::string& name, Args&&... args) : Image(device, name, args...), sampler(new VkSampler) {
    constexpr VkSamplerCreateInfo samplerInfo {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
      .mipLodBias = 0,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 0,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = std::numeric_limits<float>::min(),
      .maxLod = std::numeric_limits<float>::max(),
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE
    };
    if (const VkResult result = vkCreateSampler(device->device, &samplerInfo, nullptr, sampler)) GraphicsInstance::showError(result, "failed to create sampler.");
  }
};
