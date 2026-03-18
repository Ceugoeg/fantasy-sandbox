#pragma once

#include "entities/BaseEntity.h"
#include "core/World.h"

namespace fsb {
namespace entities {

// 鸭子类型（Duck Typing），接受多个协议
template <typename... Policies>
class Entity : public BaseEntity, public Policies... {
public:
    Entity(int id, int x, int y) : BaseEntity(id, x, y) {}

    // override 基类的纯虚函数，履行契约
    void update(core::World& world) override {
        // 自身不包含任何逻辑，将决策权与世界感知权全权委托给策略
        (this->Policies::execute(*this, world), ...);
    }
};

} // namespace entities
} // namespace fsb
