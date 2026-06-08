#pragma once

#include <glm/vec3.hpp>

#include "TagComponent.hpp"
#include "src/EntityComponentSystem/ComponentColumn.hpp"
#include "src/EntityComponentSystem/AoSColumn.hpp"
#include "src/RenderEngine/MeshGroup/MeshGroup.hpp"

/**
 * This class should store all components used in the given game. Each one should have an alias and a case in the
 * createComponentColumn and loadComponent functions.
 */
class Components {
public:
    using Position = AoSColumn<0, glm::vec3>;
    using MeshGroupComponent = AoSColumn<1, MeshGroup>;
    using Visible = TagComponent<2>;

    /**
     * Creates an empty column for storing component data of the type of the given id.
     *
     * @param id the id of the type of ComponentColumn to be created
     * @return a unique ptr to the new ComponentColumn
     */
    [[nodiscard]] static std::unique_ptr<ComponentColumn> createComponentColumn(ComponentId id);

    /**
     * Add a single component from the JSON to the given component column
     *
     * @param jsonData The data for the component in the JSON. Must be formatted correctly. Generally should be passed
     * using locateComponent.
     * @param column The ComponentColumn to be added to
     */
    static void loadComponent(yyjson_val* jsonData, ComponentColumn &column, GraphicsDevice* graphicsDevice);
};

