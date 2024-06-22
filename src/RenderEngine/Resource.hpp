#pragma once

class Resource {
public:
  enum Type {
    Image,
    Buffer
  } type;

  explicit Resource(const Type type) : type(type) {}
  virtual ~Resource();

  [[nodiscard]] virtual void* getObject() const = 0;
  [[nodiscard]] virtual void* getView() const = 0;
};