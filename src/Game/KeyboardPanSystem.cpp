//
// Created by ethan on 6/3/2026.
//

#include "KeyboardPanSystem.hpp"

#include "Game.hpp"
#include "src/InputEngine/Input.hpp"

void KeyboardPanSystem::updateComponents() {
    std::vector<std::vector<ComponentColumn*>> components = ecs->getComponents({Components::Position::ID});
    positions.reserve(components[0].size());
    for (int i = 0; i < components[0].size(); ++i) {
        positions.push_back(static_cast<Components::Position*>(components[0][i]));
    }
};

KeyboardPanSystem::KeyboardPanSystem(ECSRegistry* ecs) : ecs(ecs) {
    updateComponents();
}
template<typename T>
std::vector<T> getAoSValues(std::vector<ComponentColumn*> componentColumns) {
    auto flattenedValues = componentColumns
        | std::views::transform([](T* out) {return *out;})
        | std::views::join;
    return std::ranges::to<std::vector>(flattenedValues);
}

void KeyboardPanSystem::onTick() {
    auto flattenedPositions = positions
        | std::views::transform([](Components::Position* pos) {return std::span{pos->data(), pos->size()};})
        | std::views::join;
    for (auto &curPos : flattenedPositions) {
        //move the player using keyboard
        if (Input::keyDown(SDLK_UP) > 0 || Input::keyDown(SDLK_W) > 0) {
            curPos.y += movementSpeed * Game::getTickTime();
        }
        if (Input::keyDown(SDLK_DOWN) > 0 || Input::keyDown(SDLK_S) > 0) {
            curPos.y -= movementSpeed * Game::getTickTime();
        }
        if (Input::keyDown(SDLK_LEFT) > 0 || Input::keyDown(SDLK_A) > 0) {
            curPos.x -= movementSpeed * Game::getTickTime();
        }
        if (Input::keyDown(SDLK_RIGHT) > 0 || Input::keyDown(SDLK_D) > 0) {
            curPos.x += movementSpeed * Game::getTickTime();
        }
    }
}