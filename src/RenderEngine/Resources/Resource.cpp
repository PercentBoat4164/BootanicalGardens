#include "Resource.hpp"

Resource::Resource(const Type type, std::shared_ptr<GraphicsDevice> device) : type(type), device(device) {}
Resource::~Resource() = default;