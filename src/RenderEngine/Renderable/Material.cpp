#include "Material.hpp"

#include <SDL_image.h>
#include <iostream>

#include "Texture.hpp"
#include "src/RenderEngine/Buffer.hpp"

#include <src/RenderEngine/CommandBuffer.hpp>

const std::byte* handleDataSource(const fastgltf::Asset& asset, const fastgltf::DataSource& source, size_t* size) {
  return std::visit(fastgltf::visitor {
        [&size](const auto& arg) -> const std::byte* {
          return reinterpret_cast<std::byte*>(*size = 0);
        },
        [&asset, &size](const fastgltf::sources::BufferView& view){
          auto& bufferView = asset.bufferViews[view.bufferViewIndex];
          *size = bufferView.byteLength;
          auto& buffer = asset.buffers[bufferView.bufferIndex];
          std::size_t dummy;
          return handleDataSource(asset, buffer.data, &dummy) + bufferView.byteOffset;
        },
        [&size](const fastgltf::sources::Array& vector){
          *size = vector.bytes.size();
          return vector.bytes.data();
        }
      }, source);
}

template<typename T> requires std::derived_from<T, fastgltf::TextureInfo> void loadTexture(const GraphicsDevice& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, std::shared_ptr<Texture>* texture, const fastgltf::Optional<T>* textureInfo) {
  if (!textureInfo->has_value()) return;
  const fastgltf::Optional<std::size_t>& imageIndex = asset.textures[textureInfo->value().textureIndex].imageIndex;
  if (!imageIndex.has_value()) return;
  const fastgltf::Image& image = asset.images[imageIndex.value()];
  std::size_t size;
  auto* bytes = const_cast<std::byte*>(handleDataSource(asset, image.data, &size));
  SDL_RWops* io = SDL_RWFromMem(bytes, static_cast<int>(size));
  SDL_Surface* surface = IMG_Load_RW(io, true);
  const std::vector textureBytes(static_cast<std::byte*>(surface->pixels), static_cast<std::byte*>(surface->pixels) + surface->w * surface->h * surface->format->BytesPerPixel);
  auto buffer = std::make_shared<Buffer>(device, std::string{image.name + " upload buffer"}, textureBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
  VkFormat format;
  switch (static_cast<SDL_PixelFormatEnum>(surface->format->format)) {
    case SDL_PIXELFORMAT_ARGB4444: format = VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT; break;
    case SDL_PIXELFORMAT_RGBA4444: format = VK_FORMAT_R4G4B4A4_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_ABGR4444: format = VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT; break;
    case SDL_PIXELFORMAT_BGRA4444: format = VK_FORMAT_B4G4R4A4_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_ARGB1555: format = VK_FORMAT_A1R5G5B5_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_RGBA5551: format = VK_FORMAT_R5G5B5A1_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_ABGR1555: format = VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR; break;
    case SDL_PIXELFORMAT_BGRA5551: format = VK_FORMAT_B5G5R5A1_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_RGB565: format = VK_FORMAT_R5G6B5_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_BGR565: format = VK_FORMAT_B5G6R5_UNORM_PACK16; break;
    case SDL_PIXELFORMAT_RGB24: format = VK_FORMAT_R8G8B8_UNORM; break;
    case SDL_PIXELFORMAT_BGR24: format = VK_FORMAT_B8G8R8_UNORM; break;
    case SDL_PIXELFORMAT_RGBA32: format = VK_FORMAT_R8G8B8A8_SNORM; break;
    case SDL_PIXELFORMAT_ABGR32: format = VK_FORMAT_A8B8G8R8_UNORM_PACK32; break;
    case SDL_PIXELFORMAT_BGRA32: format = VK_FORMAT_B8G8R8A8_UNORM; break;
    case SDL_PIXELFORMAT_RGBX32: format = VK_FORMAT_R8G8B8A8_SNORM; break;
    case SDL_PIXELFORMAT_XBGR32: format = VK_FORMAT_A8B8G8R8_UNORM_PACK32; break;
    case SDL_PIXELFORMAT_BGRX32: format = VK_FORMAT_B8G8R8A8_UNORM; break;
    case SDL_PIXELFORMAT_ARGB2101010: format = VK_FORMAT_A2R10G10B10_UNORM_PACK32; break;
    default: format = VK_FORMAT_UNDEFINED;
  }
  *texture = std::make_shared<Texture>(device, std::string{image.name}, format, VkExtent3D{static_cast<uint32_t>(surface->w), static_cast<uint32_t>(surface->h), 1U}, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  std::vector<VkBufferImageCopy> regions{{
    .bufferOffset      = 0,
    .bufferRowLength   = 0,
    .bufferImageHeight = 0,
    .imageSubresource  = VkImageSubresourceLayers {
      .aspectMask     = (*texture)->aspect(),
      .mipLevel       = 0,
      .baseArrayLayer = 0,
      .layerCount     = (*texture)->layerCount(),
    },
    .imageOffset = {},
    .imageExtent = (*texture)->extent()
  }};
  commandBuffer.record<CommandBuffer::CopyBufferToImage>(buffer, *texture, regions);
  SDL_FreeSurface(surface);
}

Material::Material(const GraphicsDevice& device, CommandBuffer& commandBuffer, const fastgltf::Asset& asset, const fastgltf::Material& material) :
    doubleSided(material.doubleSided),
    alphaMode(material.alphaMode),
    alphaCutoff(material.alphaCutoff),
    unlit(material.unlit),
    ior(material.ior),
    dispersion(material.dispersion),
    albedoFactor(material.pbrData.baseColorFactor.x(), material.pbrData.baseColorFactor.y(), material.pbrData.baseColorFactor.z(), material.pbrData.baseColorFactor.w()),
    normalFactor(material.normalTexture.has_value() ? material.normalTexture->scale : 1.f),
    occlusionFactor(material.occlusionTexture.has_value() ? material.occlusionTexture->strength : 1.f),
    emissiveFactor(material.emissiveFactor.x(), material.emissiveFactor.y(), material.emissiveFactor.z()),
    anisotropyFactor(material.anisotropy ? material.anisotropy->anisotropyStrength : 0.f),
    anisotropyRotation(material.anisotropy ? material.anisotropy->anisotropyRotation : 0.f),
    metallicFactor(material.pbrData.metallicFactor),
    roughnessFactor(material.pbrData.roughnessFactor) {
  /**@todo This could be immensely sped up using multi-threading (most lucrative option because most time is spent on the CPU in loadTexture calls) and by using a dedicated transfer queue for these operations.
   *    I refrained from doing these things for two reasons:
   *    - I do not know what the multi-threading of this game is going to look like yet, and I am not ready to make that decision (especially without Ethan).
   *    - I am aiming for speed of development. I just want to make this work for now, hence this comment reminding myself to improve this later.
   *      - Fixing the way that command buffers are handled should be almost trivial. The only tricky part is retaining all the temporary buffers until the commands have finished then properly destroying them.
   */
  loadTexture<decltype(material.pbrData.baseColorTexture)::value_type>(device, commandBuffer, asset, &albedoTexture, &material.pbrData.baseColorTexture);
  loadTexture<decltype(material.normalTexture)::value_type>(device, commandBuffer, asset, &normalTexture, &material.normalTexture);
  loadTexture<decltype(material.occlusionTexture)::value_type>(device, commandBuffer, asset, &occlusionTexture, &material.occlusionTexture);
  loadTexture<decltype(material.emissiveTexture)::value_type>(device, commandBuffer, asset, &emissiveTexture, &material.emissiveTexture);
  if (material.anisotropy) loadTexture<decltype(material.anisotropy->anisotropyTexture)::value_type>(device, commandBuffer, asset, &anisotropyTexture, &material.anisotropy->anisotropyTexture);
  loadTexture<decltype(material.pbrData.metallicRoughnessTexture)::value_type>(device, commandBuffer, asset, &metallicRoughnessTexture, &material.pbrData.metallicRoughnessTexture);
}
