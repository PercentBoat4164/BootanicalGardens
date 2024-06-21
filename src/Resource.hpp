#pragma once

class Resource {
public:
  virtual ~Resource();

  virtual void* getObject() = 0;
};