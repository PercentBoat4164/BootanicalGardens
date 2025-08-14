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

 GraphicsDevice* const device;

  explicit Resource(Type type, GraphicsDevice* const device);
  virtual ~Resource();

  [[nodiscard]] virtual void* getObject() const = 0;
  [[nodiscard]] virtual void* getView() const = 0;
};