#include "CommandBuffer.hpp"

#include "RenderPass.hpp"

void CommandBuffer::beginRenderPass(const std::shared_ptr<RenderPass>& renderPass) {
  commands.emplace(Command::BeginRenderPass, std::vector<std::shared_ptr<void>>{renderPass});
}
