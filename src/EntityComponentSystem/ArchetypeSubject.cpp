#pragma once

#include "ArchetypeSubject.hpp"

void ArchetypeSubject::addListener(const ECSUpdateCallback updateFunction, void* system) {
    listeners.emplace_back(updateFunction, system);
}

void ArchetypeSubject::removeListener(const ECSUpdateCallback updateFunction, void* system) {
    for (int i = 0; i < listeners.size(); i++) {
        auto listener = listeners[i];
        if (listener == std::pair{updateFunction, system}) {
            listeners.erase(listeners.begin() + i);
        }
    }
}

void ArchetypeSubject::update(const EntityType &archetype, const bool &created) const {
    for (const auto listener : listeners) {
        listener.first(archetype, created, listener.second);
    }
}
