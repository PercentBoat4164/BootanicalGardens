#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>
#include <yyjson.h>

#include "../EntityComponentSystem/ECSRegistry.hpp"

struct yyjson_doc;
struct yyjson_val;

/**
 * Loads levels from level files to be used by the Game class
 */
class LevelParser {
    static yyjson_doc* doc;

    static void getComponentTemplate(yyjson_val * component, yyjson_val * templateIndex) {

    }

    static void loadEntity(yyjson_val * yyjson_entity) {
        yyjson_val *component;
        yyjson_arr_iter iter = yyjson_arr_iter_with(yyjson_entity);
        while ((component = yyjson_arr_iter_next(&iter))) {
            getComponentTemplate(component, yyjson_obj_iter_get_val(component));
        }
    }

public:
    /**
    * Loads an entire level from the .json file
    *
    * @param filename The path to the .json file
    */
    static void loadLevel(const std::filesystem::path& filename, ECSRegistry& ecs) {
        //read the file, throwing an error if it is not valid
           yyjson_read_err error;
           doc = yyjson_read_file(filename.string().c_str(), YYJSON_READ_ALLOW_INF_AND_NAN | YYJSON_READ_ALLOW_COMMENTS, nullptr, &error);
           if (doc == nullptr) { /**@todo: Read `error`.*/ }

           //add each entity to the entities in Game
           yyjson_val* root = yyjson_doc_get_root(doc);
           yyjson_val* entities = yyjson_obj_get(root, "entities");
           size_t max;
           size_t i;
           yyjson_val* currEntity;
           yyjson_arr_foreach(entities, i, max, currEntity) {
             loadEntity(currEntity);
           }
    }
};




// old level parser
// /**
//  * Loads an entity from the .json file.
//  *
//  * @param name The name of the entity in the .json file
//  * @return A pointer to the newly-loaded entity
//  */
// static Entity& loadEntity(yyjson_val* entityData);
//
// /**
//  * Loads an entire level from the .json file
//  *
//  * @param filename The path to the .json file
//  */
// static void loadLevel(const std::filesystem::path& filename);