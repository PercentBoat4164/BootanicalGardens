#include "Texture.hpp"

#include "src/RenderEngine/CommandBuffer.hpp"
#include "src/RenderEngine/Resources/StagingBuffer.hpp"

#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfArray.h>

VkSampler Texture::getSampler() const {
  return *sampler;
}

std::unique_ptr<Texture> Texture::jsonGet(GraphicsDevice* device, yyjson_val* textureJSON, CommandBuffer& commandBuffer) {
  std::filesystem::path path = yyjson_get_str(yyjson_obj_get(textureJSON, "path"));
  path = (device->resourcesDirectory / "textures" / path).c_str();
  Imf::RgbaInputFile file(path.c_str());
  const Imath::Box2i dataWindow = file.dataWindow();
  const unsigned int width  = dataWindow.max.x - dataWindow.min.x + 1;
  const unsigned int height = dataWindow.max.y - dataWindow.min.y + 1;
  Imf::Array2D<Imf::Rgba> pixels(width, height);
  Imf::FrameBuffer framebuffer;
  file.setFrameBuffer(&pixels[0][0], 1, width);
  file.readPixels(dataWindow.min.y, dataWindow.max.y);
  auto texture = std::make_unique<Texture>(device, path, VK_FORMAT_R8G8B8A8_SRGB, VkExtent3D{width, height, 1}, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  struct Pixel{uint8_t r; uint8_t g; uint8_t b; uint8_t a;};
  std::vector<Pixel> data(width * height);
  for (uint32_t columnIndex = 0; columnIndex < width; ++columnIndex) {
    const Imf::Rgba* column = pixels[columnIndex];
    for (uint32_t rowIndex = 0; rowIndex < height; ++rowIndex) {
      const Imf::Rgba& pixel = column[rowIndex];
      data[rowIndex + columnIndex * width] = Pixel{
        .r = static_cast<uint8_t>(pixel.r * std::numeric_limits<decltype(Pixel::r)>::max()),
        .g = static_cast<uint8_t>(pixel.g * std::numeric_limits<decltype(Pixel::g)>::max()),
        .b = static_cast<uint8_t>(pixel.b * std::numeric_limits<decltype(Pixel::b)>::max()),
        .a = static_cast<uint8_t>(pixel.a * std::numeric_limits<decltype(Pixel::a)>::max())
      };
    }
  }
  auto* buffer  = new StagingBuffer(device, (std::string(path) + " | upload buffer").c_str(), data);
  std::vector<VkBufferImageCopy> regions{
    {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = VkImageSubresourceLayers{
        .aspectMask     = texture->getAspect(),
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = texture->getLayerCount(),
      },
      .imageOffset = {},
      .imageExtent = texture->getExtent()
    }
  };
  commandBuffer.record<CommandBuffer::CopyBufferToImage>(buffer, texture.get(), regions);
  commandBuffer.addCleanupResource(buffer);
  return texture;
}