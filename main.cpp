#include "src/Game/Game.hpp"
#include "src/Game/LevelParser.hpp"
#include "src/InputEngine/Input.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/Pipeline.hpp"
#include "src/RenderEngine/RenderPass/CollectShadowsRenderPass.hpp"
#include "src/RenderEngine/RenderPass/GBufferRenderPass.hpp"
#include "src/RenderEngine/RenderPass/RenderPass.hpp"
#include "src/RenderEngine/RenderPass/ShadowRenderPass.hpp"
#include "src/RenderEngine/Renderable/Material.hpp"
#include "src/RenderEngine/Renderable/Renderable.hpp"
#include "src/RenderEngine/Shader.hpp"
#include "src/RenderEngine/Window.hpp"

int main() {
  if (!Input::initialize()) GraphicsInstance::showSDLError();
  GraphicsInstance::create({VK_EXT_DEBUG_UTILS_EXTENSION_NAME});
  {
    GraphicsDevice graphicsDevice;

    Window window{&graphicsDevice};

    // Build the RenderGraph
    RenderGraph renderGraph{&graphicsDevice};
    renderGraph.setResolutionGroup(RenderGraph::RenderResolution, window.getResolution(), VK_SAMPLE_COUNT_1_BIT);
    renderGraph.setImage(RenderGraph::RenderColor, RenderGraph::RenderResolution, VK_FORMAT_R16G16B16A16_UNORM, false);
    renderGraph.setImage(RenderGraph::GBufferAlbedo, RenderGraph::RenderResolution, VK_FORMAT_R16G16B16A16_UNORM, true);
    renderGraph.setImage(RenderGraph::GBufferPosition, RenderGraph::RenderResolution, VK_FORMAT_R16G16B16A16_SFLOAT, true);
    renderGraph.setImage(RenderGraph::GBufferDepth, RenderGraph::RenderResolution, VK_FORMAT_D32_SFLOAT, true);
    renderGraph.setResolutionGroup(RenderGraph::ShadowResolution, VkExtent3D{1024, 1024, 1}, VK_SAMPLE_COUNT_1_BIT);
    renderGraph.setImage(RenderGraph::ShadowDepth, RenderGraph::ShadowResolution, VK_FORMAT_D32_SFLOAT, true);
    renderGraph.insert<GBufferRenderPass>();
    renderGraph.insert<ShadowRenderPass>();
    renderGraph.insert<CollectShadowsRenderPass>();

    const auto fragmentShader = std::make_shared<Shader>(&graphicsDevice, std::filesystem::canonical("../res/shaders/gbuffer.frag"));
    const auto vertexShader = std::make_shared<Shader>(&graphicsDevice, std::filesystem::canonical("../res/shaders/gbuffer.vert"));

    const auto renderable = std::make_shared<Renderable>(&graphicsDevice, std::filesystem::canonical("../res/FlightHelmet.glb"));
    const std::vector<std::shared_ptr<Mesh>>& meshes = renderable->getMeshes();
    for (const std::shared_ptr<Mesh>& mesh: meshes) {
      mesh->getMaterial()->setFragmentShader(fragmentShader);
      mesh->getMaterial()->setVertexShader(vertexShader);
    }
    graphicsDevice.meshes.insert(meshes.begin(), meshes.end());

    renderGraph.bake();

    do {
      // Make sure that the CPU is not getting too far ahead of the GPU
      VkSemaphore frameDataSemaphore = renderGraph.waitForNextFrameData();
      // Make sure that the GPU is appropriately waiting for the display (V-Sync)
      const std::shared_ptr<Image> swapchainImage = window.getNextImage(frameDataSemaphore);
      // Render (CPU issues work to GPU)
      renderGraph.update();
      renderGraph.execute(swapchainImage, window.getSemaphore());
      // Tell the GPU to show the final image when it has finished rendering this frame
      window.present();
    } while (Game::tick());
  }
  GraphicsInstance::destroy();
  return 0;
}

// A RenderGraph, when baked, produces a MalleableCommandBuffer which can be baked multiple times with different arguments.
// A MalleableCommandBuffer, when baked, produces a fully baked and ready-to-go CommandBuffer that has the arguments for that bake built-in.
// This system separates the data from the commands being executed meaning that one could feasibly execute the same RenderGraph with two different views on two different threads at the same time.
//
// Currently, this can only be achieved in a single-threaded way:
//    renderGraph.update(); renderGraph.execute(); renderGraph.update(); renderGraph.execute();
//  It should be possible to make it threadable with two renderGraphs:
//    RenderGraph renderGraph1{}; //... build renderGraph1 ...; RenderGraph renderGraph2{renderGraph}; renderGraph1.update(); renderGraph2.update(); renderGraph1.execute(); renderGraph2.execute();
//  While multiple RenderGraphs can be executed at the same time, each RenderGraph can only be executing at most once at any point in time.