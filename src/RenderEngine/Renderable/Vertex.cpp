#include "Vertex.hpp"

bool Vertex::operator==(const Vertex& other) const {
  return position == other.position && textureCoordinates0 == other.textureCoordinates0;
}