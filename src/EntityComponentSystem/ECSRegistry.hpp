#pragma once
#include <map>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <ranges>

#include "Archetype.hpp"
#include "ArchetypeSubject.hpp"

/**
 * holds archetypes and maps used to query for entities and components
 */
class ECSRegistry
{
    ArchetypeSubject archetypeSubject; // updates systems when the ArchetypeMap changes
    std::uint32_t archetypeCount = 0;
    std::uint32_t componentCount = 0;
    EntityId entityCount = 0;

    EntityId registerEntityFromArchetype(Archetype* archetype, const size_t row) {
        entityRecords.insert({entityCount, Archetype::EntityRecord{archetype, row}});
        return entityCount++;
    }

public:
    /**
     * Create a new Archetype from the component data given
     *
     * @param components the components that the archetype will store
     * @param entityIds the ids of entities to be added. Should be empty if entities are already registered. If used,
     * all entities to be added must have an ID and the indices of entitiIds must match those of components.
     * @param expectedSize the expected number of entities in the archetype
     */
    Archetype* registerArchetype(std::vector<std::unique_ptr<ComponentColumn>> components, const std::uint32_t expectedSize = 0, std::vector<EntityId> entityIds = {});

    using ArchetypeMap = std::unordered_map<Archetype*, size_t>; // used to get whether and in which row a component is stored by an archetype
    std::unordered_map<ComponentId, ArchetypeMap> componentIndex; // used to get all archetype locations of a given component
    std::map<EntityType, Archetype> archetypeIndex; // used to get an archetype by its components
    std::unordered_map<EntityId, Archetype::EntityRecord> entityRecords; // location of an entity within an archetype

    /**
     * Tells whether an entity has a specific component
     *
     * @param entity the entity to be checked
     * @param component the component to be looked for
     * @return whether the entity includes the component
     */
    bool hasComponent(const EntityId entity, const ComponentId component);

    /**
     * Get the ComponentColumn for a given entity.
     *
     * @return The component that holds data for a given entity
     */
    ComponentColumn* getComponent(EntityId entity, ComponentId component);

    /**
     * Get the ComponentColumn for a given entity type.
     *
     * @return The component of a specific archetype
     */
    ComponentColumn* getComponent(const EntityType& entityType, ComponentId component);

    /**
     * Gets all the ComponentColumns (and therefore all entity data) of the given EntityType
     *
     * @return The components of a specific archetype
     */
    std::vector<std::unique_ptr<ComponentColumn>>* getComponents(const EntityType& entityType);

    /**
     * Registers entities of a single entityType, giving them ids and placing them into an archetype.
     *
     * @param entityType the ids components held by the entities
     * @param componentData the ComponentColumns of the entities
     */
    void registerEntities(const EntityType& entityType, std::vector<std::unique_ptr<ComponentColumn>> componentData);

    /**
      * Deletes the given entity, removing it from the archetype and registry
      *
      * @param entity the entity to be deleted
      */
     void deleteEntity(EntityId entity);

    /**
     * Deletes an archetype including all entities held within it
     *
     * @param type the EntityType of the archetype to be deleted
     */
    void deleteArchetype(EntityType type);

    /**
     * Gets the IDs of every entity of a specific type
     *
     * @param entityType the types of components stored by this entity
     */
    std::vector<EntityId> getEntityIds(const EntityType& entityType);

    /**
     * Add a listener to update whenever the ArchetypeMap is updated. This can keep systems' entity info up to date in
     * the event that an archetype they would operate on is added or removed
     *
     * @param updateFunction The function to be called whenever an Archetype is added or removed
     */
    void addArchetypeListener(const ECSUpdateCallback updateFunction);

    /**
     * Remove a listener from the update function.
     *
     * @param updateFunction The function to no longer be called when an Archetype is added or removed
     */
    void removeArchetypeListener(ECSUpdateCallback updateFunction);

    /**
     * Add a specific component to an Entity. Moves the Entity to another Archetype, creating a new one if needed.
     *
     * @param entity The entity a component will be added to
     * @param component The component to be added. Should include data for the entity being modified
     * @param expectedSize The number of elements to preallocate for the new archetype, if created
     */
    void addComponent(const EntityId entity, std::unique_ptr<ComponentColumn> component, const std::uint32_t expectedSize = 1);

    /**
     * Remove a specific component to an Entity. Moves the Entity to another Archetype, creating a new one if needed.
     *
     * @param entity The entity a component will be added to
     * @param component The component to be removed.
     * @param expectedSize The number of elements to preallocate for the new archetype, if created
     */
    void removeComponent(const EntityId& entity, ComponentId component, const std::uint32_t expectedSize = 1);
};
