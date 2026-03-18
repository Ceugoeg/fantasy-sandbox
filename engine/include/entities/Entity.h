#pragma once

#include "entities/BaseEntity.h"
#include "core/World.h"
#include <string>

namespace fsb {
namespace entities {

template <typename... Policies>
class Entity : public BaseEntity, public Policies... {
public:
    Entity(int id, const std::string& type_name, int x, int y) 
        : BaseEntity(id, type_name, x, y) {}

    void update(core::World& world) override {
        (this->Policies::execute(*this, world), ...);
    }
};

} // namespace entities
} // namespace fsb