#pragma once

#include "src/EntityComponentSystem/ComponentColumn.hpp"
#include "src/EntityComponentSystem/AoSColumn.hpp"
#include "src/Tools/Json/Json_glm.hpp"
#include "src/RenderEngine/MeshGroup/MeshGroup.hpp"

#include <filesystem>
#include <iostream>

struct yyjson_doc;
struct yyjson_val;

class JsonParser {

    yyjson_doc* doc = nullptr;
    yyjson_val* componentTemplates = nullptr;
    yyjson_val* archetypes = nullptr;
    GraphicsDevice* graphicsDevice = nullptr;

    using Position = AoSColumn<0, glm::vec3>;
    // using MeshGroupComponent = AoSColumn<1, MeshGroup>;

    /**
     * Load an archetype to be added to the ECS
     */
    void loadArchetype(yyjson_val* archetype);

    void loadLevel(const std::filesystem::path& path);
    void loadLevel(yyjson_val* jsonData);

    void loadComponent(const ComponentId id, const std::size_t index, ComponentColumn& column) const;

    /**
     * Add a single component from the JSON to the given component column
     *
     * @param jsonData the json object representing the component
     * @param column the column of data
     */
    void loadComponent(yyjson_val* jsonData, ComponentColumn& column) const {
        switch (column.getId()) {
            case Position::ID: // position
                column.add(new Position({Tools::jsonGet<glm::vec3>(jsonData)}));
                break;
            case 1: // meshGroup
            {
                std::vector<MeshGroup> groups;
                groups.reserve(1);
                groups.emplace_back(graphicsDevice, jsonData);
                column.add(new MeshGroupColumn(std::move(groups)));
                break;
            }
            default:
                std::cout << "Unknown component type" << std::endl;
        }
    }
};