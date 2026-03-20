#pragma once

namespace fsb {
namespace core {
    class World;
}
namespace entities {
    class BaseEntity;

namespace policies {

class IPolicy {
public:
    virtual ~IPolicy() = default;

    // 核心执行接口：每一帧被实体调用
    virtual void execute(BaseEntity& entity, core::World& world) = 0;

    // 当策略被压入栈或弹出栈时触发（可选实现，用于初始化状态或清理现场）
    virtual void onEnter(BaseEntity& /* entity */, core::World& /* world */) {}
    virtual void onExit(BaseEntity& /* entity */, core::World& /* world */) {}
};

} // namespace policies
} // namespace entities
} // namespace fsb