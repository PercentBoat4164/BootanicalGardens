# Bootanical Gardens

# What Does a `Component` Do?

# Outline
```
res
|-- models
|   |-- ghost.gltf
|
|-- sounds
|   |-- boo.wav
|
|-- levels
    |-- japanese.level  // JSON that can be used to generate a group of Entities

src
|-- Component.cpp
|-- Entity.cpp  // Manages a group of Components
|-- RenderEngine
|   |-- Renderable.cpp  // A model, material, textures, and other rendering parameters | Inherits from Component.cpp
|
|-- Game
|   |-- Level.cpp  // Manages loading and storing data relevant to levels
|   |-- Components
|       |-- EnemyAi.cpp  // Controls behaviour of enemies | Inherits from Component.cpp
|       |-- PlayerController.cpp  // Controls behaviour of player | Inherits from Component.cpp
|       |-- NpcAi.cpp  // Controls behaviour of npcs | Inherits from Component.cpp
|
|-- Audio
    |-- Noisy.cpp  // A sound, and other audio parameters | Inherits from Component.cpp
```