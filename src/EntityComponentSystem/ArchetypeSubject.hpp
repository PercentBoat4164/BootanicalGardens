#pragma once

#include "Archetype.hpp"

#include <vector>
#include <functional>

using ECSUpdateCallback = void (*)(const EntityType &, bool, void*); // a function called when the archetypeMap is updated

/**
 * Calls listener functions when an Archetype is created/filled or emptied/destroyed
 */
struct ArchetypeSubject {
    std::vector<std::pair<ECSUpdateCallback, void*>> listeners{}; // the listener functions to be called when an update occurs

    /**
     * Start listening to this subject
     *
     * @param updateFunction the function to be added
     */
    void addListener(ECSUpdateCallback updateFunction, void* system);

    /**
     * Stop listening to this subject
     *
     * @param updateFunction the function to be removed
     */
    void removeListener(ECSUpdateCallback updateFunction, void* system);

    /**
     * run all the listener update functions
     *
     * @param archetype the type of archetype created
     * @param created true if the archetype was created, false if removed
     */
    void update(const EntityType &archetype, const bool &created) const;
};