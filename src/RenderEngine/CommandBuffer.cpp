#include "CommandBuffer.hpp"

#include "RenderPass.hpp"

CommandBuffer::BeginRenderPass::BeginRenderPass(const RenderPass& renderPass) : renderPass(renderPass) {}
CommandBuffer::CopyImage::CopyImage(const Image& dst, const Image& src, const VkExtent3D extent, const VkOffset3D dstOffset, const VkOffset3D srcOffset) : dst(dst), src(src), extent(extent), dstOffset(dstOffset), srcOffset(srcOffset) {}
