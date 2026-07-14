#include "MeshUpdatingSystem.hpp"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <ranges>

void MeshUpdatingSystem::updateComponents() {
    std::vector<std::vector<ComponentColumn*>> components = ecs->getComponents({Components::Position::ID, Components::MeshGroupComponent::ID, Components::Visible::ID});
    positions.reserve(components[0].size());
    meshGroups.reserve(components[0].size());
    for (int i = 0; i < components[0].size(); ++i) {
        positions.push_back(static_cast<Components::Position*>(components[0][i]));
        meshGroups.push_back(static_cast<Components::MeshGroupComponent*>(components[1][i]));
    }
}

void MeshUpdatingSystem::transformMeshGroup(const MeshGroup& meshGroup, const glm::vec3& position) {
    const glm::mat4 entityTransform = glm::translate(glm::identity<glm::mat4>(), position);
    for (auto& [mesh, references] : meshGroup.meshes) {
        bool& stale = mesh->stale;
        for (const Mesh::InstanceReference& reference: references) {
            glm::mat4 modelMatrix = *reference.modelInstanceID;
            *reference.modelInstanceID = entityTransform * reference.perInstanceDataID->originalModelMatrix;
            stale |= mesh->instances[reference.material].stale |= modelMatrix != *reference.modelInstanceID;
        }
    }
}

MeshUpdatingSystem::MeshUpdatingSystem(GraphicsDevice* device, ECSRegistry* ecs) : device(device), ecs(ecs) {
    updateComponents();
    ecs->addArchetypeListener(&onECSChange, this);
}

void MeshUpdatingSystem::onTick() {
    auto flattenedPositions = positions
        | std::views::transform([](Components::Position* pos) -> Components::Position& {return *pos;})
        | std::views::join;
    auto flattenedMeshGroups = meshGroups
        | std::views::transform([](Components::MeshGroupComponent* out) -> Components::MeshGroupComponent& {return *out;})
        | std::views::join;
    auto curPos = flattenedPositions.begin();
    for (MeshGroup& meshGroup: flattenedMeshGroups) {
        transformMeshGroup(meshGroup, *curPos);
        ++curPos;
    }
}

void MeshUpdatingSystem::onECSChange(const EntityType & entityType, const bool created, void* system) {
    if (entityType.contains(Components::Position::ID) && entityType.contains(Components::MeshGroupComponent::ID) && entityType.contains(Components::Visible::ID)) static_cast<MeshUpdatingSystem*>(system)->updateComponents();
}