#include "Texture.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/Resources/StagingBuffer.hpp"

#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChannelListAttribute.h>
#include <OpenEXR/ImfArray.h>

VkSampler Texture::getSampler() const {
  return *sampler;
}

std::unique_ptr<Texture> Texture::jsonGet(GraphicsDevice* device, yyjson_val* textureJSON, CommandBuffer& commandBuffer) {
  std::filesystem::path path = yyjson_get_str(yyjson_obj_get(textureJSON, "path"));
  const bool linearColor = strcmp(yyjson_get_str(yyjson_obj_get(textureJSON, "colorSpace")), "linear") == 0;
  path = (device->resourcesDirectory / "textures" / path).c_str();
  std::string pathStr = path.string();
  Imf::RgbaInputFile file(pathStr.c_str());
  const Imath::Box2i dataWindow = file.header().dataWindow();
  const unsigned int width  = dataWindow.max.x - dataWindow.min.x + 1;
  const unsigned int height = dataWindow.max.y - dataWindow.min.y + 1;
  Imf::Array2D<Imf::Rgba> pixels(width, height);
  file.setFrameBuffer(&pixels[0][0], 1, width);
  file.readPixels(dataWindow.min.y, dataWindow.max.y);
  auto texture = std::make_unique<Texture>(device,
#if !defined(NDEBUG)
    pathStr,
#endif
    linearColor ? VK_FORMAT_R16G16B16A16_UNORM : VK_FORMAT_B8G8R8A8_SRGB, VkExtent3D{width, height, 1}, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

  auto* buffer = new StagingBuffer(device, (pathStr + " | upload buffer").c_str(), vkuFormatElementSize(texture->getFormat()) * width * height);
  const std::shared_ptr<Buffer::BufferMapping> mapping = buffer->map();

  struct Pixel_B8G8R8A8_SRGB{std::uint8_t b, g, r, a;};
  struct Pixel_R16G16B16A16_UNORM{std::uint16_t r, g, b, a;};

  if (linearColor) {
    auto* data = static_cast<Pixel_R16G16B16A16_UNORM*>(mapping->data);
    for (uint32_t columnIndex = 0; columnIndex < width; ++columnIndex) {
      const Imf::Rgba* column = pixels[columnIndex];
      for (uint32_t rowIndex = 0; rowIndex < height; ++rowIndex) {
        const Imf::Rgba& pixel = column[rowIndex];
        data[rowIndex + columnIndex * height] = Pixel_R16G16B16A16_UNORM{
          .r = static_cast<decltype(Pixel_R16G16B16A16_UNORM::r)>(pixel.r * std::numeric_limits<decltype(Pixel_R16G16B16A16_UNORM::r)>::max()),
          .g = static_cast<decltype(Pixel_R16G16B16A16_UNORM::g)>(pixel.g * std::numeric_limits<decltype(Pixel_R16G16B16A16_UNORM::g)>::max()),
          .b = static_cast<decltype(Pixel_R16G16B16A16_UNORM::b)>(pixel.b * std::numeric_limits<decltype(Pixel_R16G16B16A16_UNORM::b)>::max()),
          .a = static_cast<decltype(Pixel_R16G16B16A16_UNORM::a)>(pixel.a * std::numeric_limits<decltype(Pixel_R16G16B16A16_UNORM::a)>::max())
        };
      }
    }
  } else {
    auto* data = static_cast<Pixel_B8G8R8A8_SRGB*>(mapping->data);
    for (uint32_t columnIndex = 0; columnIndex < width; ++columnIndex) {
      const Imf::Rgba* column = pixels[columnIndex];
      for (uint32_t rowIndex = 0; rowIndex < height; ++rowIndex) {
        const Imf::Rgba& pixel = column[rowIndex];
        data[rowIndex + columnIndex * height] = Pixel_B8G8R8A8_SRGB{
          .b = static_cast<decltype(Pixel_B8G8R8A8_SRGB::b)>(pixel.b * std::numeric_limits<decltype(Pixel_B8G8R8A8_SRGB::b)>::max()),
          .g = static_cast<decltype(Pixel_B8G8R8A8_SRGB::g)>(pixel.g * std::numeric_limits<decltype(Pixel_B8G8R8A8_SRGB::g)>::max()),
          .r = static_cast<decltype(Pixel_B8G8R8A8_SRGB::r)>(pixel.r * std::numeric_limits<decltype(Pixel_B8G8R8A8_SRGB::r)>::max()),
          .a = static_cast<decltype(Pixel_B8G8R8A8_SRGB::a)>(pixel.a * std::numeric_limits<decltype(Pixel_B8G8R8A8_SRGB::a)>::max())
        };
      }
    }
  }

  commandBuffer.record<CommandBuffer::CopyBufferToImage>(buffer, texture.get());
  CommandBuffer::PipelineBarrier::ImageMemoryBarrier imageMemoryBarrier{
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .pNext = nullptr,
    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = texture.get(),
    .subresourceRange = texture->getWholeRange()
  };
  commandBuffer.record<CommandBuffer::PipelineBarrier>(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, std::span<CommandBuffer::PipelineBarrier::MemoryBarrier>{}, std::span<CommandBuffer::PipelineBarrier::BufferMemoryBarrier>{}, std::span{&imageMemoryBarrier, 1});
  commandBuffer.addCleanupResource(buffer);
  return texture;
}