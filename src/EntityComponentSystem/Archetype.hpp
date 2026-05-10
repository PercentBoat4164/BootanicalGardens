#pragma once
#include <cstdint>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
#include <ranges>

#include "ComponentColumn.hpp"

// Unique 32-bit unsigned integer IDs for each archetype, entity, and component.
using ArchetypeId = std::uint32_t;
using EntityId = std::uint32_t;
using EntityType = std::set<ComponentId>;

/**
 * A table holding data for all entities of a specific EntityType. Each row represents an entity, and each column is a
 * specific component of entities in this table.
 */
class Archetype {
public:
    // What row of which archetype an entity can be found
    struct EntityRecord {
        Archetype* archetype;  // todo: Figure out if I should really be a pointer. Perhaps a reference is better
        size_t row;
    };

    Archetype(const ArchetypeId archetypeId, const EntityType &entityType, const std::vector<EntityId> &entities, std::vector<std::unique_ptr<ComponentColumn>> components);

    ArchetypeId id; // Unique id
    EntityType componentIds; // Ids of each component within this archetype
    std::vector<EntityId> entityIds; // the ids of entities stored by this archetype
    std::vector<std::unique_ptr<ComponentColumn>> componentTable; // component data
    std::unordered_map<ComponentId, Archetype*> edges; // Archetypes reached by adding/removing one component to/from this entity

    /**
     * Stores an entity in this Archetype. Must have the same components as this Archetype!
     *
     * @param entityComponents the data for the entity
     * @return The record or the location of the new entity
     */
    EntityRecord addEntity(std::vector<std::unique_ptr<ComponentColumn>>&& entityComponents, EntityId entityId);

    /**
     * Stores multiple entities in the archetype
     *
     * @param entityComponents the data table for the entities
     * @param entityId the ids of entities to be added
     * @return
     */
    std::vector<EntityRecord> addEntities(std::vector<std::unique_ptr<ComponentColumn>>&& entityComponents, const std::vector<EntityId> &entityId);

    /**
     * Remove an entity from the archetype.
     *
     * @param entityRecord The location of the entity to be removed
     */
    void removeEntity(const EntityRecord& entityRecord);

    /**
     * Remove an entity from the archetype.
     *
     * @param entityRecord The location of the entity to be removed
     */
     std::vector<std::unique_ptr<ComponentColumn>> pullEntity(const EntityRecord& entityRecord);

    /**
     * Gets the archetype with a single component added or created. If the component is not already in the archetype,
     * assumed to be an addition. If it is, assumed to be a subtraction.
     *
     * @param component the component being added or removed
     * @return a pointer to the desired archetype. Nullptr if the archetype is not found via edges.
     */
    Archetype* getEdge(const ComponentId component);

    /**
     * Add an edge to the archetype graph
     *
     * @param archetype the archetype reached by adding or removing a given component to the EntityType
     * @param component the component to be added or removed
     */
    void addEdge(Archetype* archetype, ComponentId component);

    /**
     * Erase an edge from the archetype graph
     *
     * @param component the component link to be erased
     */
    void removeEdge(ComponentId component);
};