#include "src/EntityComponentSystem/Components.hpp"
#include "src/Game/Game.hpp"
#include "src/InputEngine/Input.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"
#include "src/RenderEngine/GraphicsInstance.hpp"
#include "src/RenderEngine/MeshGroup/MeshGroup.hpp"
#include "src/RenderEngine/Pipeline/Pipeline.hpp"
#include "src/RenderEngine/RenderPass/GBufferRenderPass.hpp"
#include "src/RenderEngine/RenderPass/ShadeRenderPass.hpp"
#include "src/RenderEngine/RenderPass/ShadowRenderPass.hpp"
#include "src/RenderEngine/RenderPass/SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass.hpp"
#include "src/RenderEngine/RenderPass/SubpixelMorphologicalAntiAliasingEdgeDetectionRenderPass.hpp"
#include "src/RenderEngine/RenderPass/SubpixelMorphologicalAntiAliasingNeighborhoodBlendingRenderPass.hpp"
#include "src/RenderEngine/Window.hpp"
#include "src/EntityComponentSystem/JsonParser.hpp"
#include "src/Game/KeyboardPanSystem.hpp"
#include "src/RenderEngine/MeshGroup/MeshUpdatingSystem.hpp"

//todo: easier way to add components

int main() {
  if (!Input::initialize()) GraphicsInstance::showSDLError();
  GraphicsInstance::create({
#if defined(VK_EXT_debug_utils)
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
  });
  {
    GraphicsDevice graphicsDevice{std::filesystem::canonical("../res/graphicsData.json")};

    // Declare the window
    Window window{&graphicsDevice};

    // Build the RenderGraph
    RenderGraph renderGraph{&graphicsDevice};
    renderGraph.settings.renderResolution = window.getResolution();
    renderGraph.insert<ShadowRenderPass>();
    renderGraph.insert<GBufferRenderPass>();
    renderGraph.insert<ShadeRenderPass>();
    // renderGraph.insert<SubpixelMorphologicalAntiAliasingEdgeDetectionRenderPass>();
    // renderGraph.insert<SubpixelMorphologicalAntiAliasingBlendingWeightRenderPass>();
    // renderGraph.insert<SubpixelMorphologicalAntiAliasingNeighborhoodBlendingRenderPass>();

    ECSRegistry ecs{};
    JsonParser parser(graphicsDevice);
    parser.readAndLoadLevel("../res/levels/Level1Restructured.json", ecs);
    MeshUpdatingSystem meshUpdatingSystem(&graphicsDevice, &ecs);
    KeyboardPanSystem keyboardPanSystem(&ecs);
    std::vector<EntityId> toggleableEntities = {0};
    renderGraph.bake();
    bool visibleToggle = false;
    do {
      meshUpdatingSystem.onTick();
      keyboardPanSystem.onTick();

      if (Input::keyPressed(SDLK_V)) {
        if (visibleToggle) {
          for (EntityId curEntity : toggleableEntities) {
            ecs.removeComponent(curEntity, Components::Visible::ID);
          }
          visibleToggle = false;
        } else {
          for (EntityId curEntity : toggleableEntities) {
            ecs.addComponent(curEntity, std::make_unique<Components::Visible>(1));
          }
          visibleToggle = true;
        }
      }
      if (Input::keyPressed(SDLK_C)) {
        parser.readAndLoadLevel("../res/levels/Level1Restructured.json", ecs);
        if (visibleToggle) {
          ecs.addComponent(ecs.getEntitiesOfType({0, 1}).back(), std::make_unique<Components::Visible>(1));
        }
      }
      if (Input::keyPressed(SDLK_Z)) {
        if (!toggleableEntities.empty()) {
          ecs.deleteEntity(toggleableEntities.back());
          toggleableEntities.pop_back();
        }
      }

      graphicsDevice.update();
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