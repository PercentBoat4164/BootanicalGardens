#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>
#include <cassert>

#include "ComponentColumn.hpp"

// Unique 32-bit unsigned integer IDs for each archetype, entity, and component.
using ArchetypeId = std::uint32_t;
using EntityId = std::uint32_t;
using EntityType = std::set<ComponentId>;

class ECSRegistry; // forward declaration for friendship

/**
 * A table holding data for all entities of a specific EntityType. Each row represents an entity, and each column is a
 * specific component of entities in this table.
 */
class Archetype {
private:
    std::vector<EntityId> entityIds; // the ids of entities stored by this archetype
    std::vector<std::unique_ptr<ComponentColumn>> componentTable; // component data
    std::unordered_map<ComponentId, Archetype*> edges; // Archetypes reached by adding/removing one component to/from this entity
public:
    friend class ECSRegistry;

    // What row of which archetype an entity can be found
    struct EntityRecord {
        Archetype* archetype;
        size_t row;
    };

    Archetype(const ArchetypeId &archetypeId, EntityType entityType, const std::vector<EntityId> &entities, std::vector<std::unique_ptr<ComponentColumn>> components);

    const ArchetypeId id; // Unique id
    const EntityType componentIds; // Ids of each component within this archetype

    /**
     * Get which entities are stored by this archetype
     *
     * @return the vector of the ids of entities stored by this archetype
     */
    [[nodiscard]] const std::vector<EntityId>& getEntityIds() const;

    /**
     * Get a reference to the table of components and entities stored by this archetype.
     *
     * @return the vector of ComponentColumns stored by this archetype
     */
    [[nodiscard]] const std::vector<std::unique_ptr<ComponentColumn>>& getComponentTable() const;

    /**
     * Get archetypes found by adding or removing a component from this archetype. This map is populated lazily by
     * the ECSRegistry, so it may not be complete. //todo: give user ability to add edges?
     * @return the map of edges connected to this archetype
     */
    [[nodiscard]] const std::unordered_map<ComponentId, Archetype*>& getEdges() const;

    /**
     * Get a pointer to a ComponentColumn stored by this Archetype. Returns nullptr if this archetype does not have the
     * desired ComponentColumn.
     *
     * @param componentId The id of the component to be gotten
     * @return A pointer to the desired component
     * @todo consider changing componentIds to a presorted vector<ComponentId> to make this faster than linear time
     */
    ComponentColumn* getComponent(const ComponentId& componentId);

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
     * @return a vector of EntityRecords for each new entity stored
     */
    std::vector<EntityRecord> addEntities(std::vector<std::unique_ptr<ComponentColumn>>&& entityComponents, const std::vector<EntityId> &entityId);

    /**
     * Remove an entity from the archetype, destroying the data.
     *
     * @param entityRecord The location of the entity to be removed
     */
    std::optional<EntityRecord> removeEntity(const EntityRecord &entityRecord);

    /**
     * Remove an entity from the archetype.
     *
     * @param entityRecord The location of the entity to be removed
     * @return a pair containing the entity and the EntityRecord of an entity that was moved to its place.
     */
     std::pair<std::vector<std::unique_ptr<ComponentColumn>>, std::optional<Archetype::EntityRecord>> pullEntity(const EntityRecord& entityRecord);

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