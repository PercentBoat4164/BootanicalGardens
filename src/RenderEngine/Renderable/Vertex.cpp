#include "Vertex.hpp"

bool Vertex::operator==(const Vertex& other) const {
  return position == other.position && normal == other.normal && tangent == other.tangent && textureCoordinates0 == other.textureCoordinates0 && color0 == other.color0;
}