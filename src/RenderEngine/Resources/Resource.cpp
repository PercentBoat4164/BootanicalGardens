#include "Resource.hpp"

Resource::Resource(const Type type, GraphicsDevice* const device) : type(type), device(device) {}
Resource::~Resource() = default;