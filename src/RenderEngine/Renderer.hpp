#pragma once

#include "Renderable/Renderable.hpp"

#include "src/Tools/Hive.hpp"

class Renderer {
    Hive<Renderable, 128> renderables;
};
