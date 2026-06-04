#pragma once
#include "src/EntityComponentSystem/Components.hpp"
#include "src/EntityComponentSystem/ECSRegistry.hpp"

class KeyboardPanSystem {
    ECSRegistry* ecs;
    std::vector<Components::Position*> positions;
    float movementSpeed = 0.5f;
    void updateComponents();
public:
    KeyboardPanSystem(ECSRegistry* ecs);
    void onTick();
};