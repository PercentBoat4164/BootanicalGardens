#include "Texture.hpp"

VkSampler Texture::getSampler() const {
  return *sampler;
}