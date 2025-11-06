#include "Vertex.hpp"

std::unordered_map<std::string, VkVertexInputRate> Vertex::inputRates = {
      {"inPosition", VK_VERTEX_INPUT_RATE_VERTEX},
      {"inTextureCoordinates", VK_VERTEX_INPUT_RATE_VERTEX},
      {"inNormal", VK_VERTEX_INPUT_RATE_VERTEX},
      {"inTangent", VK_VERTEX_INPUT_RATE_VERTEX},
      {"inModelMatrix", VK_VERTEX_INPUT_RATE_INSTANCE},
      {"inMaterialID", VK_VERTEX_INPUT_RATE_INSTANCE}
};