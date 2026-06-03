#pragma once

#include "Archetype.hpp"

#include <vector>
#include <functional>

//todo: switch to more robust (std::function?)
//using ECSUpdateCallBack2 = std::function<void (const EntityType &, const bool)>;
typedef void (*ECSUpdateCallback)(const EntityType &, const bool); // a function called when the archetypeMap is updated

/**
 * Calls listener functions when an Archetype is created/filled or emptied/destroyed
 */
struct ArchetypeSubject {
    std::vector<ECSUpdateCallback> listeners{}; // the listener functions to be called when an update occurs

    /**
     * Start listening to this subject
     *
     * @param updateFunction the function to be added
     */
    void addListener(ECSUpdateCallback updateFunction);

    /**
     * Stop listening to this subject
     *
     * @param updateFunction the function to be removed
     */
    void removeListener(ECSUpdateCallback updateFunction);

    /**
     * run all the listener update functions
     *
     * @param archetype the type of archetype created
     * @param created true if the archetype was created, false if removed
     */
    void update(const EntityType &archetype, const bool &created) const;
};