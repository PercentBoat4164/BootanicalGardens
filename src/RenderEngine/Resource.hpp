#pragma once

#include <memory>

class GraphicsDevice;
class Buffer;

class Resource : public std::enable_shared_from_this<Resource> {
public:
  enum Type {
    Image,
    Buffer
  } type;

  const std::shared_ptr<GraphicsDevice> device;

  explicit Resource(Type type, const std::shared_ptr<GraphicsDevice>& device);
  virtual ~Resource();

  [[nodiscard]] virtual void* getObject() const = 0;
  [[nodiscard]] virtual void* getView() const = 0;
};