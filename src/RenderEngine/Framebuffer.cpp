#include "Framebuffer.hpp"

#include "GraphicsDevice.hpp"
#include "Resources/Image.hpp"

#include <memory>
#include <vector>
#include <volk/volk.h>

Framebuffer::Framebuffer(GraphicsDevice* const device, const std::vector<const Image*>& images, VkRenderPass renderPass) : device(device),
                                                                                                                                     images(images),
                                                                                                                                     extent{images.front()->getExtent().width, images.front()->getExtent().height},
                                                                                                                                     rect{0, 0, extent.width, extent.height} {
  std::vector<VkImageView> views{images.size()};
  for (uint32_t i{}; i < images.size(); ++i)
    views[i] = images[i]->getImageView();
  const VkFramebufferCreateInfo framebufferCreateInfo{
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext           = nullptr,
      .flags           = 0,
      .renderPass      = renderPass,
      .attachmentCount = static_cast<uint32_t>(views.size()),
      .pAttachments    = views.data(),
      .width           = extent.width,
      .height          = extent.height,
      .layers          = images.front()->getLayerCount()
  };
  vkCreateFramebuffer(device->device, &framebufferCreateInfo, nullptr, &framebuffer);
}

Framebuffer::~Framebuffer() {
  if (framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(device->device, framebuffer, nullptr);
    framebuffer = VK_NULL_HANDLE;
  }
}

VkFramebuffer Framebuffer::getFramebuffer() const {
  return framebuffer;
}

VkExtent2D Framebuffer::getExtent() const {
  return extent;
}

VkRect2D Framebuffer::getRect() const {
  return rect;
}

std::vector<const Image*> Framebuffer::getImages() const {
  return images;
}
