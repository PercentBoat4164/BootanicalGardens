#include "Archetype.hpp"

#include <cassert>
#include <utility>

Archetype::Archetype(const ArchetypeId &archetypeId, EntityType  entityType, const std::vector<EntityId> &entities, std::vector<std::unique_ptr<ComponentColumn>> components) : id(archetypeId), componentIds(std::move(entityType)) {
    this->componentTable = std::move(components);
    entityIds = entities;
}

const std::vector<EntityId> & Archetype::getEntityIds() const {
    return entityIds;
}

const std::vector<std::unique_ptr<ComponentColumn>> & Archetype::getComponentTable() const {
    return componentTable;
}

const std::unordered_map<ComponentId, Archetype *> & Archetype::getEdges() const {
    return edges;
}

ComponentColumn * Archetype::getComponent(const ComponentId &componentId) {
    return componentTable[std::distance(componentIds.begin(), componentIds.find(componentId))].get();
}

Archetype::EntityRecord Archetype::addEntity(std::vector<std::unique_ptr<ComponentColumn>> &&entityComponents, EntityId entityId) {
    for (int i = 0; i < entityComponents.size(); i++) {
        if (componentIds.contains(entityComponents.at(i)->getId())) {
            componentTable[i]->add(std::move(entityComponents.at(i)));
        }
    }
    entityIds.push_back(entityId);
    return EntityRecord{this, this->componentTable[0]->getLength() - 1};
}

std::vector<Archetype::EntityRecord> Archetype::addEntities(
    std::vector<std::unique_ptr<ComponentColumn>> &&entityComponents, const std::vector<EntityId> &entityId) {
    for (int i = 0; i < entityComponents.size(); i++) {
        if (componentIds.contains(entityComponents.at(i)->getId())) {
            componentTable[i]->add(std::move(entityComponents.at(i)));
        }
    }
    std::vector<EntityRecord> entityRecords;
    for (EntityId i : entityIds) {
        entityIds.push_back(entityId[i]);
        entityRecords.push_back(EntityRecord{this, this->componentTable[0]->getLength()});
    }
    return entityRecords;
}

void Archetype::removeEntity(const EntityRecord &entityRecord) {
    if (entityRecord.archetype != this) return;
    for (const auto & component : componentTable) {
        component->remove(entityRecord.row);
    }
    entityIds[entityRecord.row] = entityIds.back();
    entityIds.pop_back();
}

std::vector<std::unique_ptr<ComponentColumn>> Archetype::pullEntity(const EntityRecord &entityRecord) {
    assert(entityRecord.archetype == this);
    std::vector<std::unique_ptr<ComponentColumn>> entity;
    entity.reserve(componentTable.size());
    for (const auto & component : componentTable) {
        entity.push_back(std::move(component->getRange(entityRecord.row, 1)));
    }
    entityIds[entityRecord.row] = entityIds.back();
    entityIds.pop_back();
    return entity;
}

Archetype * Archetype::getEdge(const ComponentId component) {
    // attempt to find the target archetype using the edge and add the entity to it
    if (edges.contains(component)) {
        return edges.at(component);
    }
    return nullptr;
}

void Archetype::addEdge(Archetype *archetype, ComponentId component) {
    edges.insert({component, archetype});
}

void Archetype::removeEdge(ComponentId component) {
    edges.erase(component);
}
