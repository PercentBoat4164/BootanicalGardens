#include "src/Game/Game.hpp"
#include "src/Game/LevelParser.hpp"
#include "src/InputEngine/Input.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/RenderPass/OpaqueRenderPass.hpp"
#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Renderable.hpp"
#include "src/RenderEngine/Shader.hpp"
#include "src/RenderEngine/Window.hpp"

int main() {
  if (!Input::initialize()) GraphicsInstance::showSDLError();
  GraphicsInstance::create({});
  {
    const auto graphicsDevice = std::make_shared<GraphicsDevice>();

    Window window{graphicsDevice};

    // Build the RenderGraph
    RenderGraph renderGraph{graphicsDevice};
    renderGraph.setResolutionGroup(RenderGraph::RenderResolution, window.getResolution());
    renderGraph.setAttachment(RenderGraph::RenderColor, RenderGraph::RenderResolution, VK_FORMAT_R16G16B16A16_UNORM, VK_SAMPLE_COUNT_1_BIT);
    renderGraph.setAttachment(RenderGraph::RenderDepth, RenderGraph::RenderResolution, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT);
    std::shared_ptr<RenderPass> renderPass = *renderGraph.insert<OpaqueRenderPass>();

    const auto renderable = std::make_shared<Renderable>(graphicsDevice, std::filesystem::canonical("../res/FlightHelmet.glb"));
    for (const auto& mesh : renderable->getMeshes())
      mesh->getMaterial()->setShaders(std::vector{
        std::make_shared<Shader>(graphicsDevice, std::filesystem::canonical("../res/default.frag")),
        std::make_shared<Shader>(graphicsDevice, std::filesystem::canonical("../res/default.vert"))
      });
    renderGraph.addRenderable(renderable);

    CommandBuffer commandBuffer;
    renderGraph.bake(commandBuffer);
    graphicsDevice->executeCommandBufferImmediate(commandBuffer);

    do {
      // Make sure that the CPU is not getting too far ahead of the GPU
      VkSemaphore frameDataSemaphore = renderGraph.waitForNextFrameData();
      // Make sure that the GPU is appropriately waiting for the display (V-Sync)
      const std::shared_ptr<Image> swapchainImage = window.getNextImage(frameDataSemaphore);
      // Render (CPU issues work to GPU)
      renderGraph.update();
      VkSemaphore frameFinishedSemaphore = renderGraph.execute(swapchainImage);
      // Tell the GPU to show the final image when it has finished processing this frame
      window.present(frameFinishedSemaphore);
    } while (Game::tick());
  }
  GraphicsInstance::destroy();
  return 0;
}