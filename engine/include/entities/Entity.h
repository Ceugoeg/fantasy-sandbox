#pragma once

#include "entities/BaseEntity.h"
// 这里需要引入 World.h，因为我们要把 world 传递给 policy
#include "core/World.h"

namespace fsb {
namespace entities {

// 【核心原理：Policy-Based Design】
// 这里的 BehaviorPolicy 就是我们在编译期注入的“灵魂”。
// 只要传入的类型中包含一个形如 execute(BaseEntity&, core::World&) 的方法，
// 鸭子类型（Duck Typing）在 C++ 模板实例化的阶段就能完美契合，且没有任何虚函数调用的性能开销。
template <typename BehaviorPolicy>
class Entity : public BaseEntity {
private:
    // 实例化一个具体的策略对象
    BehaviorPolicy policy_;

public:
    // 构造函数：完美转发参数给基类 BaseEntity 以初始化 id 和坐标
    Entity(int id, int x, int y) : BaseEntity(id, x, y) {}

    // 覆盖（override）基类的纯虚函数，履行契约
    void update(core::World& world) override {
        // 自身不包含任何逻辑，将决策权与世界感知权全权委托给策略
        policy_.execute(*this, world);
    }
};

} // namespace entities
} // namespace fsb
