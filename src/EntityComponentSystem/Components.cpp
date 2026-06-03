#include "Components.hpp"

#include "src/Tools/Json/Json_glm.hpp"

std::unique_ptr<ComponentColumn> Components::createComponentColumn(const ComponentId id) {
    // There should be a case for every component that may be loaded from a Json
    switch (id) {
        case Position::ID: return std::make_unique<Position>();
        case MeshGroupComponent::ID: return std::make_unique<MeshGroupComponent>();
        default:
            return nullptr;
    }
}

// @todo: remove graphicsDevice once render engine is updated
void Components::loadComponent(yyjson_val * jsonData , ComponentColumn &column, GraphicsDevice* graphicsDevice) {
    // There should be a case for every component that may be loaded from a Json
    switch (column.getId()) {
        case Position::ID:
            column.add(std::make_unique<Position>(std::vector{Tools::jsonGet<glm::vec3>(jsonData)}));
            break;
        case MeshGroupComponent::ID: {
            std::vector<MeshGroup> groups;
            groups.reserve(1);
            groups.emplace_back(graphicsDevice, jsonData);
            column.add(std::make_unique<MeshGroupComponent>(std::move(groups)));
            break;
        }
        default:
            break;
    }
}