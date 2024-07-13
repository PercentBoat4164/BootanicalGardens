#include "CommandBuffer.hpp"

#include "RenderPass.hpp"

CommandBuffer::BeginRenderPass::BeginRenderPass(const RenderPass& renderPass) : renderPass(renderPass) {}
CommandBuffer::CopyImage::CopyImage(const Image& dst, const Image& src, VkExtent3D extent, VkOffset3D dstOffset, VkOffset3D srcOffset) : dst(dst), src(src), extent(extent), dstOffset(dstOffset), srcOffset(srcOffset) {}
