#pragma once

#include <magic_enum/magic_enum.hpp>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <queue>
#include <source_location>

class RenderPass;
class Image;

template <typename T>
consteval auto f() {
  const auto& loc = std::source_location::current();
  return loc.function_name();
}

template <typename T>
consteval std::string_view nameOf() {
  constexpr std::string_view functionName = f<T>();
  // since func_name_ is 'consteval auto f() [with T = ...]'
  // we can simply get the subrange
  // because the position after the equal will never change since
  // the same function name is used

  // another notice: these magic numbers will not work on MSVC
  return {functionName.begin() + 29, functionName.end() - 1};
}

class CommandBuffer {
  struct CommandData {};
public:
  struct BeginRenderPass : public CommandData {
    explicit BeginRenderPass(const RenderPass& renderPass);

    const RenderPass& renderPass;
  };

  struct CopyImage : public CommandData {
    CopyImage(const Image& dst, const Image& src, VkExtent3D extent, VkOffset3D dstOffset = {0, 0, 0}, VkOffset3D srcOffset = {0, 0, 0});

    const Image& dst;
    const Image& src;
    VkExtent3D extent;
    VkOffset3D dstOffset;
    VkOffset3D srcOffset;
  };

private:
  struct Command {
    enum Type {
      BeginRenderPass,
      BindDescriptorSets,
      BindPipeline,
      CopyImage,
      Dispatch,
      EndRenderPass
    } type;

    std::unique_ptr<CommandData> data;
  };

  std::deque<Command> commands;

public:
  template<typename T, typename... Args> requires std::constructible_from<T, Args...> void record(Args&&... args) {
    commands.emplace_back(magic_enum::enum_cast<Command::Type>(nameOf<T>()).value(), std::make_unique<T>(std::forward<Args>(args)...));
  }
};
