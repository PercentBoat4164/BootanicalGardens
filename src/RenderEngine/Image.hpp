#pragma once

#include "Resource.hpp"

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <string>
#include <vector>

class GraphicsDevice;
class Buffer;

class Image : public Resource {
  VmaAllocation allocation{VK_NULL_HANDLE};

  std::string _name;
  bool _shouldDestroy{true};
  VkImage _image{VK_NULL_HANDLE};
  VkFormat _format;
  VkImageAspectFlags _aspect;
  VkExtent3D _extent;
  VkImageUsageFlags _usage;
  VkImageView _view;
  uint32_t _mipLevels{1};
  uint32_t _layerCount{1};

public:
  enum Usages : VkImageUsageFlags {
    Texture = VK_IMAGE_USAGE_SAMPLED_BIT,
    Compute = VK_IMAGE_USAGE_STORAGE_BIT,
  };

  Image(const GraphicsDevice& device, std::string name, VkImage image=VK_NULL_HANDLE, VkFormat format=VK_FORMAT_UNDEFINED, VkExtent3D extent={}, VkImageUsageFlags usage=0, VkImageView view=VK_NULL_HANDLE);
  Image(const GraphicsDevice& device, std::string name, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, uint32_t mipLevels=1, VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);
  ~Image() override;

  void buildInPlace(VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, uint32_t mipLevels=1, VkSampleCountFlagBits samples=VK_SAMPLE_COUNT_1_BIT);

  [[nodiscard]] VkImage image() const;
  [[nodiscard]] VkExtent3D extent() const;
  [[nodiscard]] VkImageView view() const;
  [[nodiscard]] VkImageView view(VkComponentMapping mapping, const VkImageSubresourceRange&subresourceRange) const;
  [[nodiscard]] VkFormat format() const;
  [[nodiscard]] VkImageAspectFlags aspect() const;
  [[nodiscard]] uint32_t mipLevels() const;
  [[nodiscard]] uint32_t layerCount() const;

private:
  [[nodiscard]] void* getObject() const override;
  [[nodiscard]] void* getView() const override;

  static VkImageAspectFlags aspectFromFormat(VkFormat format) {
    switch (format) {
      case VK_FORMAT_UNDEFINED:
        return VK_IMAGE_ASPECT_NONE;
      case VK_FORMAT_R4G4_UNORM_PACK8:
      case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
      case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
      case VK_FORMAT_R5G6B5_UNORM_PACK16:
      case VK_FORMAT_B5G6R5_UNORM_PACK16:
      case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
      case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
      case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
      case VK_FORMAT_R8_UNORM:
      case VK_FORMAT_R8_SNORM:
      case VK_FORMAT_R8_USCALED:
      case VK_FORMAT_R8_SSCALED:
      case VK_FORMAT_R8_UINT:
      case VK_FORMAT_R8_SINT:
      case VK_FORMAT_R8_SRGB:
      case VK_FORMAT_R8G8_UNORM:
      case VK_FORMAT_R8G8_SNORM:
      case VK_FORMAT_R8G8_USCALED:
      case VK_FORMAT_R8G8_SSCALED:
      case VK_FORMAT_R8G8_UINT:
      case VK_FORMAT_R8G8_SINT:
      case VK_FORMAT_R8G8_SRGB:
      case VK_FORMAT_R8G8B8_UNORM:
      case VK_FORMAT_R8G8B8_SNORM:
      case VK_FORMAT_R8G8B8_USCALED:
      case VK_FORMAT_R8G8B8_SSCALED:
      case VK_FORMAT_R8G8B8_UINT:
      case VK_FORMAT_R8G8B8_SINT:
      case VK_FORMAT_R8G8B8_SRGB:
      case VK_FORMAT_B8G8R8_UNORM:
      case VK_FORMAT_B8G8R8_SNORM:
      case VK_FORMAT_B8G8R8_USCALED:
      case VK_FORMAT_B8G8R8_SSCALED:
      case VK_FORMAT_B8G8R8_UINT:
      case VK_FORMAT_B8G8R8_SINT:
      case VK_FORMAT_B8G8R8_SRGB:
      case VK_FORMAT_R8G8B8A8_UNORM:
      case VK_FORMAT_R8G8B8A8_SNORM:
      case VK_FORMAT_R8G8B8A8_USCALED:
      case VK_FORMAT_R8G8B8A8_SSCALED:
      case VK_FORMAT_R8G8B8A8_UINT:
      case VK_FORMAT_R8G8B8A8_SINT:
      case VK_FORMAT_R8G8B8A8_SRGB:
      case VK_FORMAT_B8G8R8A8_UNORM:
      case VK_FORMAT_B8G8R8A8_SNORM:
      case VK_FORMAT_B8G8R8A8_USCALED:
      case VK_FORMAT_B8G8R8A8_SSCALED:
      case VK_FORMAT_B8G8R8A8_UINT:
      case VK_FORMAT_B8G8R8A8_SINT:
      case VK_FORMAT_B8G8R8A8_SRGB:
      case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
      case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
      case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
      case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
      case VK_FORMAT_A8B8G8R8_UINT_PACK32:
      case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
      case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
      case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
      case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
      case VK_FORMAT_A2R10G10B10_UINT_PACK32:
      case VK_FORMAT_A2R10G10B10_SINT_PACK32:
      case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
      case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
      case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
      case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
      case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      case VK_FORMAT_R16_UNORM:
      case VK_FORMAT_R16_SNORM:
      case VK_FORMAT_R16_USCALED:
      case VK_FORMAT_R16_SSCALED:
      case VK_FORMAT_R16_UINT:
      case VK_FORMAT_R16_SINT:
      case VK_FORMAT_R16_SFLOAT:
      case VK_FORMAT_R16G16_UNORM:
      case VK_FORMAT_R16G16_SNORM:
      case VK_FORMAT_R16G16_USCALED:
      case VK_FORMAT_R16G16_SSCALED:
      case VK_FORMAT_R16G16_UINT:
      case VK_FORMAT_R16G16_SINT:
      case VK_FORMAT_R16G16_SFLOAT:
      case VK_FORMAT_R16G16B16_UNORM:
      case VK_FORMAT_R16G16B16_SNORM:
      case VK_FORMAT_R16G16B16_USCALED:
      case VK_FORMAT_R16G16B16_SSCALED:
      case VK_FORMAT_R16G16B16_UINT:
      case VK_FORMAT_R16G16B16_SINT:
      case VK_FORMAT_R16G16B16_SFLOAT:
      case VK_FORMAT_R16G16B16A16_UNORM:
      case VK_FORMAT_R16G16B16A16_SNORM:
      case VK_FORMAT_R16G16B16A16_USCALED:
      case VK_FORMAT_R16G16B16A16_SSCALED:
      case VK_FORMAT_R16G16B16A16_UINT:
      case VK_FORMAT_R16G16B16A16_SINT:
      case VK_FORMAT_R16G16B16A16_SFLOAT:
      case VK_FORMAT_R32_UINT:
      case VK_FORMAT_R32_SINT:
      case VK_FORMAT_R32_SFLOAT:
      case VK_FORMAT_R32G32_UINT:
      case VK_FORMAT_R32G32_SINT:
      case VK_FORMAT_R32G32_SFLOAT:
      case VK_FORMAT_R32G32B32_UINT:
      case VK_FORMAT_R32G32B32_SINT:
      case VK_FORMAT_R32G32B32_SFLOAT:
      case VK_FORMAT_R32G32B32A32_UINT:
      case VK_FORMAT_R32G32B32A32_SINT:
      case VK_FORMAT_R32G32B32A32_SFLOAT:
      case VK_FORMAT_R64_UINT:
      case VK_FORMAT_R64_SINT:
      case VK_FORMAT_R64_SFLOAT:
      case VK_FORMAT_R64G64_UINT:
      case VK_FORMAT_R64G64_SINT:
      case VK_FORMAT_R64G64_SFLOAT:
      case VK_FORMAT_R64G64B64_UINT:
      case VK_FORMAT_R64G64B64_SINT:
      case VK_FORMAT_R64G64B64_SFLOAT:
      case VK_FORMAT_R64G64B64A64_UINT:
      case VK_FORMAT_R64G64B64A64_SINT:
      case VK_FORMAT_R64G64B64A64_SFLOAT:
      case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
      case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return VK_IMAGE_ASPECT_COLOR_BIT;
      case VK_FORMAT_D16_UNORM:
      case VK_FORMAT_X8_D24_UNORM_PACK32:
      case VK_FORMAT_D32_SFLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;
      case VK_FORMAT_S8_UINT:
        return VK_IMAGE_ASPECT_STENCIL_BIT;
      case VK_FORMAT_D16_UNORM_S8_UINT:
      case VK_FORMAT_D24_UNORM_S8_UINT:
      case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
      case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
      case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
      case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
      case VK_FORMAT_BC2_UNORM_BLOCK:
      case VK_FORMAT_BC2_SRGB_BLOCK:
      case VK_FORMAT_BC3_UNORM_BLOCK:
      case VK_FORMAT_BC3_SRGB_BLOCK:
      case VK_FORMAT_BC4_UNORM_BLOCK:
      case VK_FORMAT_BC4_SNORM_BLOCK:
      case VK_FORMAT_BC5_UNORM_BLOCK:
      case VK_FORMAT_BC5_SNORM_BLOCK:
      case VK_FORMAT_BC6H_UFLOAT_BLOCK:
      case VK_FORMAT_BC6H_SFLOAT_BLOCK:
      case VK_FORMAT_BC7_UNORM_BLOCK:
      case VK_FORMAT_BC7_SRGB_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
      case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
      case VK_FORMAT_EAC_R11_UNORM_BLOCK:
      case VK_FORMAT_EAC_R11_SNORM_BLOCK:
      case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
      case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
      case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
      case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
      case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
      case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
      case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
      case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
      case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
      case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
      case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
      case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
      case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
      case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
      case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
      case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
      case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
      case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
      case VK_FORMAT_G8B8G8R8_422_UNORM:
      case VK_FORMAT_B8G8R8G8_422_UNORM:
      case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
      case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
      case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
      case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
      case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
      case VK_FORMAT_R10X6_UNORM_PACK16:
      case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
      case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
      case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
      case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
      case VK_FORMAT_R12X4_UNORM_PACK16:
      case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
      case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
      case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
      case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
      case VK_FORMAT_G16B16G16R16_422_UNORM:
      case VK_FORMAT_B16G16R16G16_422_UNORM:
      case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
      case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
      case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
      case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
      case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
      case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
      case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
      case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
      case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
      case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
      case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
      case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
      case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
      case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
      case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
      case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
      case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
        return VK_IMAGE_ASPECT_NONE;  //@todo Handle the elements of this group correctly.
      case VK_FORMAT_R16G16_SFIXED5_NV:
      case VK_FORMAT_A1B5G5R5_UNORM_PACK16_KHR:
      case VK_FORMAT_A8_UNORM_KHR:
        return VK_IMAGE_ASPECT_COLOR_BIT;
      case VK_FORMAT_MAX_ENUM:
      default:
        return VK_IMAGE_ASPECT_NONE;
    }
  }
};
