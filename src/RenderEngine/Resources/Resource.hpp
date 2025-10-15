#pragma once

class GraphicsDevice;
class Buffer;

class Resource {
public:
  enum Type {
    Image,
    Buffer
  } type;

 GraphicsDevice* const device;

  explicit Resource(Type type, GraphicsDevice* device);
  virtual ~Resource();

  [[nodiscard]] virtual void* getObject() const = 0;
  [[nodiscard]] virtual void* getView() const = 0;
};