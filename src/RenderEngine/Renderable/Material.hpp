#pragma once

#include <fastgltf/core.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


class Shader;
class Pipeline;
class Texture;
class GraphicsDevice;
class CommandBuffer;

class Material {
  std::vector<std::shared_ptr<Shader>> shaders;

  bool doubleSided = true;
  fastgltf::AlphaMode alphaMode = fastgltf::AlphaMode::Opaque;
  float alphaCutoff = 1;
  bool unlit = false;

  float ior = 1.5;
  float dispersion = 0;

  std::shared_ptr<Texture> albedoTexture{nullptr};
  glm::vec4 albedoFactor = glm::vec4(1);

  std::shared_ptr<Texture> normalTexture{nullptr};
  float normalFactor = 1;

  std::shared_ptr<Texture> occlusionTexture{nullptr};
  float occlusionFactor = 1;

  std::shared_ptr<Texture> emissiveTexture{nullptr};
  glm::vec3 emissiveFactor = glm::vec3(1);

  std::shared_ptr<Texture> anisotropyTexture{nullptr};
  float anisotropyFactor = 1;
  float anisotropyRotation = 0;

  std::shared_ptr<Texture> metallicRoughnessTexture{nullptr};
  float metallicFactor = 1;
  float roughnessFactor = 1;

public:
  Material() = default;
  Material(const std::shared_ptr<GraphicsDevice>& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Material& material);

  [[nodiscard]] bool isDoubleSided() const;
  [[nodiscard]] fastgltf::AlphaMode getAlphaMode() const;
  [[nodiscard]] float getAlphaCutoff() const;
  [[nodiscard]] bool isUnlit() const;
  [[nodiscard]] float getIor() const;
  [[nodiscard]] float getDispersion() const;
  [[nodiscard]] std::shared_ptr<Texture> getAlbedoTexture() const;
  [[nodiscard]] glm::vec4 getAlbedoFactor() const;
  [[nodiscard]] std::shared_ptr<Texture> getNormalTexture() const;
  [[nodiscard]] float getNormalFactor() const;
  [[nodiscard]] std::shared_ptr<Texture> getOcclusionTexture() const;
  [[nodiscard]] float getOcclusionFactor() const;
  [[nodiscard]] std::shared_ptr<Texture> getEmissiveTexture() const;
  [[nodiscard]] glm::vec3 getEmissiveFactor() const;
  [[nodiscard]] std::shared_ptr<Texture> getAnisotropyTexture() const;
  [[nodiscard]] float getAnisotropyFactor() const;
  [[nodiscard]] float getAnisotropyRotation() const;
  [[nodiscard]] std::shared_ptr<Texture> getMetallicRoughnessTexture() const;
  [[nodiscard]] float getMetallicFactor() const;
  [[nodiscard]] float getRoughnessRotation() const;
  [[nodiscard]] std::vector<std::shared_ptr<Shader>> getShaders() const;

  void setShaders(const std::vector<std::shared_ptr<Shader>>& shaders);
};

