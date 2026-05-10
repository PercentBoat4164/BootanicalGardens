#include "JsonParser.hpp"

#include "ECSRegistry.hpp"

std::unique_ptr<ComponentColumn> JsonParser::createComponentFromJson(yyjson_val *jsonData, const ComponentId id) const {
    // There should be a case for every component that may be loaded from a Json
    switch (id) {
        case Position::ID:
            return std::make_unique<Position>(Position({Tools::jsonGet<glm::vec3>(jsonData)}));
        case MeshGroupComponent::ID: {
            std::vector<MeshGroup> groups;
            groups.reserve(1);
            groups.emplace_back(graphicsDevice, jsonData);
            return std::make_unique<MeshGroupComponent>(std::move(groups));
        }
        default:
            std::cout << "Unknown component type" << std::endl;
        return nullptr;
    }
}

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
    // iterate over each component type
    yyjson_arr_iter componentIt = yyjson_arr_iter_with(yyjson_obj_get(archetype, "type"));
    while (yyjson_val* curColumn = yyjson_arr_iter_next(&componentIt)) {
        // for each component type, load the component for each entity
        size_t curType = yyjson_get_int(curColumn);
        components.emplace_back(std::move(loadComponentColumn(curType, curColumn)));
    }
    ecs.registerArchetype(std::move(components));
}

void JsonParser::loadComponent(const ComponentId id, const std::size_t index, ComponentColumn &column) const {
    yyjson_val* jsonData = yyjson_obj_get(componentTemplates, std::to_string(index).c_str());
    loadComponent(jsonData, column);
}

void JsonParser::loadComponent(yyjson_val *jsonData, ComponentColumn &column) const {
    column.add(std::move(createComponentFromJson(jsonData, column.getId())));
}

yyjson_val * JsonParser::locateComponent(const ComponentId type, const size_t index) const {
    yyjson_val* componentArray = yyjson_obj_get(componentTemplates, std::to_string(type).c_str());
    return yyjson_arr_get(componentArray, index);
}

std::unique_ptr<ComponentColumn> JsonParser::loadComponentColumn(ComponentId componentType, yyjson_val* column) const {
    yyjson_arr_iter it = yyjson_arr_iter_with(column);
    yyjson_val* curComponent = yyjson_arr_iter_next(&it);
    std::unique_ptr<ComponentColumn> result = std::move(createComponentFromJson(yyjson_arr_iter_next(&it), componentType));
    loadComponent(curComponent, *result);
    while ((curComponent = yyjson_arr_iter_next(&it))) {
        loadComponent(curComponent, *result);
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
