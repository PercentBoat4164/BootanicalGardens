#pragma once

#include "src/EntityComponentSystem/ComponentColumn.hpp"
#include "src/EntityComponentSystem/AoSColumn.hpp"
#include "src/Tools/Json/Json_glm.hpp"
#include "src/RenderEngine/MeshGroup/MeshGroup.hpp"

#include <filesystem>
#include <iostream>

#include "ECSRegistry.hpp"

struct yyjson_doc;
struct yyjson_val;

/**
 * Parses a .JSON containing component and archetype data.
 */
class JsonParser {

    yyjson_doc* doc = nullptr;
    yyjson_val* archetypes = nullptr;
    yyjson_val* componentTemplates = nullptr;
    GraphicsDevice* graphicsDevice = nullptr;

    /**
     *
     * @param jsonData the json object representing the component
     * @param id the ComponentId of the component to be created
     * @return a unique pointer to the created component
     */
    std::unique_ptr<ComponentColumn> createComponentFromJson(yyjson_val *jsonData, const ComponentId id) const;

    /**
     * Add a single component from the JSON to the given component column
     *
     * @param id The ComponentId of the component to be loaded
     * @param index The index of the component in the JSON
     * @param column The ComponentColumn to be added to
     */
    void loadComponent(const ComponentId id, const std::size_t index, ComponentColumn& column) const;

    /**
     * Add a single component from a JSON to the given component column
     *
     * @param jsonData the json object representing the component
     * @param column the column of data
     */
    void loadComponent(yyjson_val* jsonData, ComponentColumn& column) const;

    [[nodiscard]] yyjson_val* locateComponent(const ComponentId type, const size_t index) const;

public:
    using Position = AoSColumn<0, glm::vec3>;
    using MeshGroupComponent = AoSColumn<1, MeshGroup>;

    /**
     * Constructor
     *
     * @param device the graphics device used by the game
     */
    JsonParser(GraphicsDevice &device) {
        graphicsDevice = &device;
    };

    /**
     * Reads a .json file components will be loaded from. The file is not used directly after this is called.
     * This must be called before load unless using readAndLoad()
     *
     * @param filename the path of the file to be opened, relative to the executable
     * @return read success
     */
    bool readFile(const std::filesystem::path &filename);

    /**
     * Load an archetype to be added to the ECS
     *
     * @param archetype the yyjson object representing the archetype to be loaded
     * @param ecs the ECSRegistry to be added to
     */
    void loadArchetype(yyjson_val* archetype, ECSRegistry &ecs) const;

    /**
     * Load an entire ComponentColumn at once.
     *
     * @param componentType the id of the ComponentColumn
     * @param column the json array holding the locations of the desired components
     */
    std::unique_ptr<ComponentColumn> loadComponentColumn(const ComponentId componentType, yyjson_val* column) const;

    /**
     * Read a json file containing level data and load that data into the ECS
     *
     * @param path path to the file to be loaded from
     * @param ecs the ECSRegistry items will be added to
     */
    void readAndLoadLevel(const std::filesystem::path& path, ECSRegistry& ecs);

    /**
     * Load data from the last read file into the ECS. readFile should be called before this
     *
     * @param ecs the ECSRegistry items will be added to
     */
    void loadLevel(ECSRegistry& ecs) const;
};