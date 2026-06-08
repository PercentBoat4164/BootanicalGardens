#include "ECSRegistry.hpp"

#include <algorithm>

std::vector<Archetype*> ECSRegistry::getArchetypesContaining(const EntityType& entityType) {
    const ComponentId firstComponent = *entityType.begin();
    const auto archetypes = componentIndex.find(firstComponent);
    if (archetypes == componentIndex.end()) return {};
    std::vector<Archetype*> result{};
    for (const auto& [archetype, componentRow] : archetypes->second) {
        if (std::ranges::includes(archetype->componentIds.begin(), archetype->componentIds.end(), entityType.begin(), entityType.end())) {
            result.push_back(archetype);
        }
    }
    return result;
}

Archetype * ECSRegistry::registerArchetype(std::vector<std::unique_ptr<ComponentColumn>> components, const std::size_t expectedSize, std::vector<EntityId> entityIds) {
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

std::vector<std::unique_ptr<ComponentColumn>> * ECSRegistry::getArchetype(const EntityType &entityType) {
    const auto archetype = archetypeIndex.find(entityType);
    if (archetype == archetypeIndex.end()) return nullptr;
    return &archetype->second.componentTable;
}

// todo: consider way to make iterating over ComponentColumns of the same type easier
std::vector<std::vector<ComponentColumn *>> ECSRegistry::getComponents(const EntityType &entityType) {
    std::vector<Archetype*> archetypes = getArchetypesContaining(entityType); // get all archetypes containing the given components
    std::vector result(entityType.size(), std::vector<ComponentColumn *>(archetypes.size()));
    // for each archetype containing the desired ComponentColumns...
    for (size_t archetype_i = 0; archetype_i < archetypes.size(); archetype_i++) {
        // find the address of the given components within the archetype and append them to the result table
        auto curComponentId = entityType.begin();
        for (size_t j = 0; j < entityType.size(); j++) {
            result[j][archetype_i] = archetypes[archetype_i]->getComponent(*curComponentId);
            ++curComponentId;
        }
    }
    return result;
}

void ECSRegistry::registerEntities(const EntityType &entityType, std::vector<std::unique_ptr<ComponentColumn>> componentData) {
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

std::vector<EntityId> ECSRegistry::getEntitiesOfType(const EntityType &entityType) {
    // find the correct archetype
    const auto archetype = archetypeIndex.find(entityType);
    if (archetype == archetypeIndex.end()) return {};
    return archetype->second.entityIds;
}

std::vector<EntityId> ECSRegistry::getEntitiesWith(const EntityType& entityType) {
    const ComponentId firstComponent = *entityType.begin();
    const auto archetypes = componentIndex.find(firstComponent);
    if (archetypes == componentIndex.end()) return {};
    std::vector<EntityId> result;
    result.reserve(archetypes->second.size());
    for (const auto& [archetype, componentRow] : archetypes->second) {
        if (std::ranges::includes(archetype->componentIds.begin(), archetype->componentIds.end(), entityType.begin()++, entityType.end())) {
            result.append_range(archetype->entityIds);
        }
    }
    return result;
}

void ECSRegistry::addArchetypeListener(const ECSUpdateCallback updateFunction, void* system) {
    archetypeSubject.addListener(updateFunction, system);
}

void ECSRegistry::removeArchetypeListener(ECSUpdateCallback updateFunction, void* system) {
    archetypeSubject.removeListener(updateFunction, system);
}

void ECSRegistry::addComponent(const EntityId entity, std::unique_ptr<ComponentColumn> component, const std::size_t expectedSize) {
    // get the source archetype
    const auto entityRecord = entityRecords.find(entity);
    if (entityRecord == entityRecords.end()) return; // if the given entity doesn't exist, do nothing
    Archetype* archetype = entityRecord->second.archetype; // initially the source archetype, then reassigned to the target archetype when found
    assert(!archetype->componentIds.contains(component->getId())); // ensure we are not adding an already existing component

    // get a buffer of the entity data from the source archetype and add the new component to it
    std::vector<std::unique_ptr<ComponentColumn>> entityBuf = archetype->pullEntity(entityRecord->second);
    entityBuf.push_back(std::move(component));

    // attempt to find the target archetype using the edge and add the entity to it
    if (archetype->getEdge(entityBuf.back()->getId()) != nullptr) {
        archetype = archetype->getEdge(entityBuf.back()->getId());
        entityRecords[entity] = archetype->addEntity(std::move(entityBuf), entity);
        return;
    }

    //if not found, attempt to find it using the archetypeIndex
    EntityType entityType = archetype->componentIds;
    entityType.insert(entityBuf.back()->getId());
    auto it = archetypeIndex.find(entityType);
    if (it != archetypeIndex.end()) {
        archetype = &it->second;
        entityRecords[entity] = archetype->addEntity(std::move(entityBuf), entity);
        return;
    }

    //otherwise, create the target archetype
    const ComponentId id = entityBuf.back()->getId();
    registerArchetype(std::move(entityBuf), expectedSize, {entity});
    entityRecord->second.archetype->addEdge(archetype, id);
}

void ECSRegistry::removeComponent(const EntityId &entity, ComponentId component, const std::size_t expectedSize) {
    // get the source archetype
    const auto entityRecord = entityRecords.find(entity);
    if (entityRecord == entityRecords.end()) return; // if the given entity doesn't exist, do nothing
    Archetype* archetype = entityRecord->second.archetype; // initially the source archetype, then reassigned to the target archetype when found
    assert(archetype->componentIds.contains(component)); // ensure we are not removing a component that doesn't exist

    // get a buffer of the entity data from the source archetype
    std::vector<std::unique_ptr<ComponentColumn>> entityBuf = std::move(archetype->pullEntity(entityRecord->second));

    // attempt to find the target archetype using the edge and add the entity to it
    if (archetype->getEdge(component) != nullptr) {
        archetype = archetype->getEdge(component);
        entityRecords[entity] = archetype->addEntity(std::move(entityBuf), entity);
        return;
    }

    //if not found, attempt to find it using the archetypeIndex
    EntityType entityType = archetype->componentIds;
    entityType.erase(component);
    auto it = archetypeIndex.find(entityType);
    if (it != archetypeIndex.end()) {
        entityRecord->second.archetype->addEdge(&it->second, component);
        archetype = &it->second;
        entityRecords[entity] = archetype->addEntity(std::move(entityBuf), entity);
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
