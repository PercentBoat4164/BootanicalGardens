#pragma once
#include "src/EntityComponentSystem/ECSRegistry.hpp"
#include <deque>

class ECSRegistry;
class JsonParser;

class HelmetManager {
    ECSRegistry* ecs;
    JsonParser* parser;
    bool visibleToggle = false;
    std::deque<EntityId> helmetInstances = std::deque<EntityId>();
public:
    HelmetManager(ECSRegistry* ecs, JsonParser* parser) : ecs(ecs), parser(parser) {}
    void onTick();
    void loadHelmet();
};
