#pragma once

#include "ECSRegistry.hpp"

Archetype * ECSRegistry::registerArchetype(std::vector<std::unique_ptr<ComponentColumn>> components,
    const std::uint32_t expectedSize, std::vector<EntityId> entityIds) {

    //std::vector<EntityId> entities(components.front()->getLength());
    std::set<ComponentId> ids;
    for (const auto& component : components) {
        ids.insert(component->getId());
    }

    // check the entityId vector and initialize it if needed
    if (!entityIds.empty()) {
        assert(entityIds.size() == components.front()->getLength());
    } else {
        entityIds.reserve(components.front()->getLength());
        for (int i = 0; i < components.front()->getLength(); i++) {
            entityIds.push_back(entityCount + i);
        }
        entityCount += entityIds.size();
    }

    // create the archetype and add it to archetypeIndex
    Archetype& archetype = archetypeIndex.emplace(std::piecewise_construct, std::forward_as_tuple(ids), std::forward_as_tuple(archetypeCount, ids, entityIds, std::move(components))).first->second;
    for (int i = 0; i < archetype.componentTable.size(); i++) {
        componentIndex[archetype.componentTable[i]->getId()].insert({&archetype, i});
    }
    ++archetypeCount;

    // register entities given with the archetype
    for (int i = 0; i < archetype.componentTable[0]->getLength(); i++) {
        entityRecords[entityIds[i]] = Archetype::EntityRecord{&archetype, static_cast<size_t>(i)};
    }
    archetypeSubject.update(archetype.componentIds, true);
    return &archetype;
}

bool ECSRegistry::hasComponent(const EntityId entity, const ComponentId component) {
    const auto i = entityRecords.find(entity);
    if (i != entityRecords.end()) { return false; }
    const auto j = i->second.archetype->componentIds.find(component);
    return j != i->second.archetype->componentIds.end();
}

ComponentColumn * ECSRegistry::getComponent(EntityId entity, ComponentId component) {
    // find the location of the entity
    const auto entityRecord = entityRecords.find(entity);
    if (entityRecord == entityRecords.end()) return nullptr;

    // find the location of the component
    const auto archetypeMap = componentIndex.find(component);
    if (archetypeMap == componentIndex.end()) return nullptr;

    // find the component record in the correct entity
    const auto componentRecord = archetypeMap->second.find(entityRecord->second.archetype);
    if (componentRecord == archetypeMap->second.end()) return nullptr;

    return entityRecord->second.archetype->componentTable[componentRecord->second].get();
}

ComponentColumn * ECSRegistry::getComponent(const EntityType &entityType, ComponentId component) {
    // find the correct archetype
    const auto archetype = archetypeIndex.find(entityType);
    if (archetype == archetypeIndex.end()) return nullptr;

    // find the correct component
    const auto componentLocation = componentIndex.find(component);
    if (componentLocation == componentIndex.end()) return nullptr;

    const auto componentRow = componentLocation->second.find(&archetype->second);
    if (componentRow == componentLocation->second.end()) return nullptr;

    return &*archetype->second.componentTable[componentRow->second];
}

std::vector<std::unique_ptr<ComponentColumn>> * ECSRegistry::getComponents(const EntityType &entityType) {
    const auto archetype = archetypeIndex.find(entityType);
    if (archetype == archetypeIndex.end()) return nullptr;
    return &archetype->second.componentTable;
}

void ECSRegistry::registerEntities(const EntityType &entityType,
    std::vector<std::unique_ptr<ComponentColumn>> componentData) {
    const auto archetypeIt = archetypeIndex.find(entityType);

    // if no archetype exists, create it
    if (archetypeIt == archetypeIndex.end()) {
        registerArchetype(std::move(componentData));
        return;
    }

    // add entities to the archetype
    std::vector<EntityId> entityIds;
    entityIds.reserve(componentData.front()->getLength());
    for (size_t i = 0; i < componentData.front()->getLength(); i++) {
        entityIds[i] = entityCount++;
    }
    const std::vector<Archetype::EntityRecord> record = archetypeIt->second.addEntities(std::move(componentData), entityIds);

    // add entities to the entityRecords
    for (std::size_t i = 0; i < record.size(); ++i) {
        entityRecords[entityIds[i]] = record[i];
    }
}

void ECSRegistry::deleteEntity(EntityId entity) {
    // find the location of the entity
    const auto entityRecord = entityRecords.find(entity);
    if (entityRecord == entityRecords.end()) return;
    entityRecord->second.archetype->removeEntity(entityRecord->second);
    entityRecords.erase(entity);
}

void ECSRegistry::deleteArchetype(EntityType type) {
    // find the archetype to be deleted
    const auto archetype = archetypeIndex.find(type);
    if (archetype == archetypeIndex.end()) return; // if the archetype doesn't exist, do nothing

    // remove links to this archetype on the Archetype graph
    for (auto edge : archetype->second.edges) {
        edge.second->removeEdge(edge.first);
    }

    // remove entities from register
    for (auto entity : archetype->second.entityIds) {
        entityRecords.erase(entity);
    }

    // delete the archetype and remove it from the registry
    for (auto component : archetype->second.componentIds) {
        componentIndex.find(component)->second.erase(&archetype->second);
    }
    archetypeIndex.erase(archetype); // where the archetype is actually stored (and therefore deleted)

    // tell the archetype listeners the tale of destruction
    archetypeSubject.update(type, false);
}

std::vector<EntityId> ECSRegistry::getEntityIds(const EntityType &entityType) {
    // find the correct archetype
    const auto archetype = archetypeIndex.find(entityType);
    if (archetype == archetypeIndex.end()) return {};
    return archetype->second.entityIds;
}

void ECSRegistry::addArchetypeListener(const ECSUpdateCallback updateFunction) {
    archetypeSubject.addListener(updateFunction);
}

void ECSRegistry::removeArchetypeListener(ECSUpdateCallback updateFunction) {
    archetypeSubject.removeListener(updateFunction);
}

void ECSRegistry::addComponent(const EntityId entity, std::unique_ptr<ComponentColumn> component,
    const std::uint32_t expectedSize) {
    // get the source archetype
    const auto entityRecord = entityRecords.find(entity);
    if (entityRecord == entityRecords.end()) return; // if the given entity doesn't exist, do nothing
    Archetype* archetype = entityRecord->second.archetype; // initially the source archetype, then reassigned to the target archetype when found
    assert(!archetype->componentIds.contains(component->getId())); // ensure we are not adding an already existing component

    // create a buffer for the entity data
    std::vector<std::unique_ptr<ComponentColumn>> entityBuf; // buffer for data to be moved to the target archetype
    entityBuf.reserve(archetype->componentIds.size() + 1);

    // get the entity data from the source archetype
    entityBuf = archetype->pullEntity(entityRecord->second);

    // attempt to find the target archetype using the edge and add the entity to it
    if (archetype->getEdge(component->getId()) != nullptr) {
        archetype = archetype->getEdge(component->getId());
        entityRecords[entity] = archetype->addEntity(std::move(entityBuf), entity);
        return;
    }

    //if not found, attempt to find it using the archetypeIndex
    EntityType entityType = archetype->componentIds;
    entityType.insert(component->getId());
    auto it = archetypeIndex.find(entityType);
    if (it != archetypeIndex.end()) {
        archetype = &it->second;
        entityRecords[entity] = archetype->addEntity(std::move(entityBuf), entity);
        return;
    }

    //otherwise, create the target archetype
    registerArchetype(std::move(entityBuf), expectedSize, {entity});
    entityRecord->second.archetype->addEdge(archetype, component->getId());
}

void ECSRegistry::removeComponent(const EntityId &entity, ComponentId component, const std::uint32_t expectedSize) {
    // get the source archetype
    const auto entityRecord = entityRecords.find(entity);
    if (entityRecord == entityRecords.end()) return; // if the given entity doesn't exist, do nothing
    Archetype* archetype = entityRecord->second.archetype; // initially the source archetype, then reassigned to the target archetype when found
    assert(archetype->componentIds.contains(component)); // ensure we are not removing a component that doesn't exist

    // create a buffer for the entity data
    std::vector<std::unique_ptr<ComponentColumn>> entityBuf; // buffer for data to be moved to the target archetype
    entityBuf.reserve(archetype->componentIds.size());

    // get the entity data from the source archetype.
    entityBuf = std::move(archetype->pullEntity(entityRecord->second));

    // attempt to find the target archetype using the edge and add the entity to it
    if (archetype->getEdge(component) != nullptr) {
        archetype = archetype->getEdge(component);
        archetype->addEntity(std::move(entityBuf), entity);
        return;
    }

    //if not found, attempt to find it using the archetypeIndex
    EntityType entityType = archetype->componentIds;
    entityType.erase(component);
    auto it = archetypeIndex.find(entityType);
    if (it != archetypeIndex.end()) {
        entityRecord->second.archetype->addEdge(&it->second, component);
        archetype = &it->second;
        archetype->addEntity(std::move(entityBuf), entity);
        return;
    }

    // otherwise, create the target archetype
    // erase the unused component
    for (int i = 0; i < entityBuf.size(); i++) {
        if (entityBuf[i]->getId() == component) {
            entityBuf.erase(entityBuf.begin() + i);
        }
    }
    registerArchetype(std::move(entityBuf), expectedSize, {entity});
    archetype->addEdge(entityRecords.find(entity)->second.archetype, component);
}
