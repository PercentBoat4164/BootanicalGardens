#include <simdjson.h>

#include "HelmetManager.hpp"
#include "src/InputEngine/Input.hpp"
#include "src/EntityComponentSystem/Components.hpp"
#include "src/EntityComponentSystem/JsonParser.hpp"

void HelmetManager::onTick() {
    if (Input::keyPressed(SDLK_V)) {
        if (visibleToggle) {
            for (EntityId curEntity : ecs->getEntitiesOfType({0, 1, 2})) {
                ecs->removeComponent(curEntity, Components::Visible::ID);
            }
            visibleToggle = false;
        } else {
            for (EntityId curEntity : ecs->getEntitiesOfType({0, 1})) {
                ecs->addComponent(curEntity, std::make_unique<Components::Visible>(1));
            }
            visibleToggle = true;
        }
    }
    if (Input::keyPressed(SDLK_C)) {
        loadHelmet();
    }
    if (Input::keyPressed(SDLK_Z)) {
        if (visibleToggle && !ecs->getEntitiesOfType({0, 1, 2}).empty()) {
            if (auto column = ecs->getComponentLocation(helmetInstances.front(), Components::MeshGroupComponent::ID)) {
                auto* meshGroup = static_cast<Components::MeshGroupComponent*>(column->second);
                meshGroup->at(column->first).removeInstance();
                ecs->deleteEntity(helmetInstances.front());
                helmetInstances.pop_front();
            }
        }
    }
}

void HelmetManager::loadHelmet() {
    parser->readAndLoadLevel("../res/levels/Level1Restructured.json", *ecs);
    helmetInstances.push_back(ecs->getEntitiesOfType({0, 1}).back());
    if (visibleToggle) {
        ecs->addComponent(ecs->getEntitiesOfType({0, 1}).back(), std::make_unique<Components::Visible>(1));
    }
}