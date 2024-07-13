#pragma once
#include "DescriptorAllocator.hpp"
#include "Image.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Shader.hpp"
#include "ShaderUsage.hpp"

#include <vector>

namespace vkb {struct Swapchain;}
class Window;
class GraphicsDevice;

class Renderer {
  constexpr static unsigned int FRAME_OVERLAP = 2;

  class PerFrameData {
    const GraphicsDevice& device;

  public:
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    VkSemaphore renderSemaphore{VK_NULL_HANDLE};
    VkSemaphore swapchainSemaphore{VK_NULL_HANDLE};
    VkFence renderFence{VK_NULL_HANDLE};

    explicit PerFrameData(const GraphicsDevice& device);

    ~PerFrameData();
  };
  std::vector<PerFrameData> frames;
  [[nodiscard]] const PerFrameData& getPerFrameData() const;

  class OneTimeSubmit {
  public:
    class CommandBuffer {
      const GraphicsDevice& device;
      VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
      bool inUse{};

    public:
      CommandBuffer(const GraphicsDevice& device, VkCommandPool commandPool);

      CommandBuffer begin();
      VkFence finish();

      explicit operator VkCommandBuffer() const;
      explicit operator bool() const;
    };

  private:
    const GraphicsDevice& device;
    VkCommandPool commandPool{VK_NULL_HANDLE};
    std::vector<CommandBuffer> commandBuffers{};

  public:
    CommandBuffer get();

    explicit OneTimeSubmit(const GraphicsDevice& device);
  };

  const GraphicsDevice& device;

  uint64_t frameNumber{};
  OneTimeSubmit oneTimeSubmit;

  std::vector<Shader> shaders;
  std::vector<ShaderUsage> shaderUsages;
  std::vector<Pipeline> pipelines;
  std::vector<RenderPass> renderPasses;
  Image drawImage;

public:
  DescriptorAllocator descriptorAllocator;

  explicit Renderer(const GraphicsDevice& device);

  void setup(const std::vector<Image>& swapchainImages);

  [[nodiscard]] VkSemaphore waitForFrameData() const;
  [[nodiscard]] std::vector<VkSemaphore> render(uint32_t swapchainIndex, Image& swapchainImage) const;
};