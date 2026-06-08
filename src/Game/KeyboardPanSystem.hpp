#pragma once
#include "src/EntityComponentSystem/Components.hpp"
#include "src/EntityComponentSystem/ECSRegistry.hpp"

class KeyboardPanSystem {
    ECSRegistry* ecs;
    std::vector<Components::Position*> positions;
    float movementSpeed = 0.5f;
    bool visibleToggle = false;
    void updateComponents();
public:
    KeyboardPanSystem(ECSRegistry* ecs);
    void onTick();
    /**
     * Used to listen for ECS changes to know when the component vectors need to be updated
     *
     * @param entityType the archetype created or destroyed
     * @param created whether it was created or destroyed
     */
    static void onECSChange(const EntityType & entityType, bool created, void* system);
};