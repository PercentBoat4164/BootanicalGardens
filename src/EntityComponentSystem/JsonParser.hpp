#pragma once

#include "src/EntityComponentSystem/ComponentColumn.hpp"
#include "src/EntityComponentSystem/ECSRegistry.hpp"
#include "src/RenderEngine/MeshGroup/MeshGroup.hpp"
#include "src/EntityComponentSystem/Components.hpp"

#include <filesystem>

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

    [[nodiscard]] yyjson_val* locateComponent(const ComponentId type, const size_t index) const;

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

public:
    /**
     * Constructor
     *
     * @param device the graphics device used by the game
     */
    JsonParser(GraphicsDevice &device) {
        graphicsDevice = &device;
    };

    /**
     * Reads a .json file components will be loaded from.
     * Allows data to be read from a file separately from parsing it; to parse immediately, use readAndLoadLevel.
     *
     * @param filename the path of the file to be opened, relative to the executable
     * @return read success
     */
    bool readFile(const std::filesystem::path &filename);

    /**
     * Read a json file containing level data and load that data into the ECS
     *
     * @param path path to the file to be loaded from
     * @param ecs the ECSRegistry items will be added to
     */
    void readAndLoadLevel(const std::filesystem::path& path, ECSRegistry& ecs);

    /**
     * Load data from the last read file into the ECS. readFile() should be called before this.
     *
     * @param ecs the ECSRegistry items will be added to
     */
    void loadLevel(ECSRegistry& ecs) const;
};