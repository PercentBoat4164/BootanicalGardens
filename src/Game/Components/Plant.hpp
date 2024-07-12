#include "src/Component.hpp"

#include "src/Entity.hpp"
#include "src/Game/Game.hpp"

#include <memory>
#include <utility>

class Plant : public Component {
public:
  Plant(std::string name, std::shared_ptr<Entity>& entity);
  ~Plant() override = default;

  void onTick() override;
};