#pragma once

class GraphicsDevice;

class Resource {
public:
  enum Type {
    Image,
    Buffer
  } type;

  const GraphicsDevice& device;

  explicit Resource(const Type type, const GraphicsDevice& device) : type(type), device(device) {}
  virtual ~Resource();

  [[nodiscard]] virtual void* getObject() const = 0;
  [[nodiscard]] virtual void* getView() const = 0;
};