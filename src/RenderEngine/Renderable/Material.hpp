#pragma once

#include <fastgltf/core.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Texture;
class GraphicsDevice;
class CommandBuffer;

class Material {
  bool doubleSided;
  fastgltf::AlphaMode alphaMode;
  float alphaCutoff;
  bool unlit;

  float ior;
  float dispersion;

  std::shared_ptr<Texture> albedoTexture{nullptr};
  glm::vec4 albedoFactor;

  std::shared_ptr<Texture> normalTexture{nullptr};
  float normalFactor;

  std::shared_ptr<Texture> occlusionTexture{nullptr};
  float occlusionFactor;

  std::shared_ptr<Texture> emissiveTexture{nullptr};
  glm::vec3 emissiveFactor;

  std::shared_ptr<Texture> anisotropyTexture{nullptr};
  float anisotropyFactor;
  float anisotropyRotation;

  std::shared_ptr<Texture> metallicRoughnessTexture{nullptr};
  float metallicFactor;
  float roughnessFactor;

public:
  Material(const GraphicsDevice& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Material& material);
};

