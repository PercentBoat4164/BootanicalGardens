#include "Json_glm.hpp"

template<>
glm::vec3 Tools::jsonGet<glm::vec3>(yyjson_val *jsonData, const std::string_view key) {
    glm::vec3 out;
    jsonData = yyjson_obj_get(jsonData, key.data());
    if (jsonData == nullptr) return {};
    assert(yyjson_arr_size(jsonData) == 3);
    out.x = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 0)));
    out.y = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 1)));
    out.z = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 2)));
    return out;
}

template<>
glm::quat Tools::jsonGet<glm::quat>(yyjson_val *jsonData, const std::string_view key) {
    glm::quat out;
    jsonData = yyjson_obj_get(jsonData, key.data());
    if (jsonData == nullptr) return {};
    assert(yyjson_arr_size(jsonData) == 4);
    out.x = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 0)));
    out.y = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 1)));
    out.z = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 2)));
    out.w = static_cast<float>(yyjson_get_num(yyjson_arr_get(jsonData, 3)));
    return out;
}

template<>
glm::mat4 Tools::jsonGet<glm::mat4>(yyjson_val *jsonData, const std::string_view key) {
    glm::mat4 out;
    jsonData = yyjson_obj_get(jsonData, key.data());
    if (jsonData == nullptr) return {};
    assert(yyjson_arr_size(jsonData) == 4);

    //add the contents of the json array to the glm mat4
    std::size_t max_row;
    std::size_t i;
    yyjson_val *col;
    yyjson_arr_foreach(jsonData, i, max_row, col) {
        assert(yyjson_arr_size(col) == 4);
        std::size_t max_col;
        std::size_t j;
        yyjson_val *item;
        yyjson_arr_foreach(col, j, max_col, item) {
            out[static_cast<int>(j)][static_cast<int>(i)] = static_cast<float>(yyjson_get_num(item));
        }
    }
    return out;
}

