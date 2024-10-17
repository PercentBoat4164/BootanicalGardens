#pragma once

#include <fastgltf/core.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

class Texture;
class GraphicsDevice;

class Material {
  bool doubleSided;
  fastgltf::AlphaMode alphaMode;
  float alphaCutoff;
  bool unlit;

  float ior;
  float dispersion;

  Texture* albedoTexture{nullptr};
  glm::vec4 albedoFactor;

  Texture* normalTexture{nullptr};
  float normalFactor;

  Texture* occlusionTexture{nullptr};
  float occlusionFactor;

  Texture* emissiveTexture{nullptr};
  glm::vec3 emissiveFactor;

  Texture* anisotropyTexture{nullptr};
  float anisotropyFactor;
  float anisotropyRotation;

  Texture* metallicRoughnessTexture{nullptr};
  float metallicFactor;
  float roughnessFactor;

public:
  Material(const GraphicsDevice& device, const fastgltf::Asset& asset, const fastgltf::Material& material);
  ~Material();
};

