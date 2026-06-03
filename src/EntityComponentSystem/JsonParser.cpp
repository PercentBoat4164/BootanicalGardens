#include "JsonParser.hpp"

bool JsonParser::readFile(const std::filesystem::path &filename) {
    yyjson_read_err error;
    doc = yyjson_read_file(filename.string().c_str(), YYJSON_READ_ALLOW_INF_AND_NAN | YYJSON_READ_ALLOW_COMMENTS, nullptr, &error);
    yyjson_val* root = yyjson_doc_get_root(doc);
    if (doc == nullptr) return error.code;
    archetypes = yyjson_obj_get(root, "archetypes");
    componentTemplates = yyjson_obj_get(root, "components");
    if (archetypes == nullptr || componentTemplates == nullptr) return false;
    return !error.code; // code == 0 -> success
}

void JsonParser::loadArchetype(yyjson_val *archetype, ECSRegistry &ecs) const {
    std::vector<std::unique_ptr<ComponentColumn>> components;
    components.reserve(yyjson_arr_size(archetype));

    // for each component column, create the component column itself then populate it
    yyjson_val* componentTypes = yyjson_obj_get(archetype, "type"); // componentIDs of each componentColumn
    yyjson_val* componentIndicesByType = yyjson_obj_get(archetype, "componentColumns"); // which components to get of each type

    size_t idx, max;
    yyjson_val *curComponentType;
    yyjson_arr_foreach(componentTypes, idx, max, curComponentType) {
        ComponentId type = yyjson_get_uint(curComponentType);
        components.emplace_back(loadComponentColumn(type, yyjson_obj_get(componentIndicesByType, std::to_string(type).c_str())));
    }
    ecs.registerArchetype(std::move(components));
}

yyjson_val * JsonParser::locateComponent(const ComponentId type, const size_t index) const {
    yyjson_val* componentArray = yyjson_obj_get(componentTemplates, std::to_string(type).c_str());
    return yyjson_arr_get(componentArray, index);
}

std::unique_ptr<ComponentColumn> JsonParser::loadComponentColumn(const ComponentId componentType, yyjson_val* column) const {
    std::unique_ptr<ComponentColumn> result = Components::createComponentColumn(componentType);
    std::size_t idx, max;
    yyjson_val *curComponent;
    yyjson_arr_foreach(column, idx, max, curComponent) {
        Components::loadComponent(locateComponent(componentType, yyjson_get_uint(curComponent)), *result, graphicsDevice);
    }
    return result;
}

void JsonParser::readAndLoadLevel(const std::filesystem::path& path, ECSRegistry& ecs) {
    readFile(path);
    loadLevel(ecs);
}

void JsonParser::loadLevel(ECSRegistry& ecs) const {
    yyjson_arr_iter archetypeIt = yyjson_arr_iter_with(archetypes);
    while (yyjson_val* curArchetype = yyjson_arr_iter_next(&archetypeIt)) {
        loadArchetype(curArchetype, ecs);
    }
};
