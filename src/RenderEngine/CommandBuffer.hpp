#pragma once

#include "Resource.hpp"

#include <memory>
#include <queue>

class RenderPass;

class CommandBuffer {
  struct Command {
    enum Type {
      BeginRenderPass,
      BindDescriptorSets,
      BindPipeline,
      Dispatch,
      EndRenderPass
    } type;

    std::vector<std::shared_ptr<void>> resources;
  };

  std::queue<Command> commands;

public:
  void beginRenderPass(const std::shared_ptr<RenderPass>& renderPass);
};
