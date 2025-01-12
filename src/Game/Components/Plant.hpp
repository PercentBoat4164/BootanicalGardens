#include "src/Component.hpp"

#include "src/Entity.hpp"
#include "src/Game/Game.hpp"

#include <memory>
#include <utility>

class Plant : public Component {
public:
  Plant(std::uint64_t id, Entity& entity);
  ~Plant() override = default;

  static std::shared_ptr<Component> create(std::uint64_t id, Entity& entity, yyjson_val* obj);

  void onTick() override;
};