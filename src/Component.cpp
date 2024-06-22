#include "Component.hpp"

Component::~Component() = default;

std::string Component::getName() {
    return name;
}
