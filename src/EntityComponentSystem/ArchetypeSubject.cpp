#pragma once

#include "ArchetypeSubject.hpp"

void ArchetypeSubject::addListener(const ECSUpdateCallback updateFunction) {
    listeners.push_back(updateFunction);
}

void ArchetypeSubject::removeListener(const ECSUpdateCallback updateFunction) {
    ECSUpdateCallback listener;
    for (int i = 0; i < listeners.size(); i++) {
        listener = listeners[i];
        if (listener == updateFunction) {
            listeners.erase(listeners.begin() + i);
        }
    }
}

void ArchetypeSubject::update(const EntityType &archetype, const bool &created) const {
    for (const ECSUpdateCallback listener : listeners) {
        listener(archetype, created);
    }
}
