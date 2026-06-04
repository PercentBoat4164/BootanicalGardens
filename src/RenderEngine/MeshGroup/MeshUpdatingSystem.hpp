#pragma once
#include "MeshGroup.hpp"
#include "src/EntityComponentSystem/Components.hpp"
#include "src/EntityComponentSystem/ECSRegistry.hpp"
#include "src/RenderEngine/GraphicsDevice.hpp"


/**
 * Keeps Mesh transformations aligned with position info in the ECS
 */
class MeshUpdatingSystem {
    void updateComponents();
    void transformMeshGroup(const MeshGroup& meshGroup, const glm::vec3& position);
public:
    GraphicsDevice* device;
    ECSRegistry* ecs;
    std::vector<Components::MeshGroupComponent*> meshGroups;
    std::vector<Components::Position*> positions;

    MeshUpdatingSystem(GraphicsDevice* device, ECSRegistry* ecs);

    /**
     * Occurs every game tick
     */
    void onTick();

    /**
     * Used to listen for ECS changes to know when the component vectors need to be updated
     *
     * @param entityType the archetype created or destroyed
     * @param created whether it was created or destroyed
     */
    static void onECSChange(const EntityType & entityType, bool created, void* system);
};
