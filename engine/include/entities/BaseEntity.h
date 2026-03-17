#pragma once

#include <string>

// 前向声明 World 类
namespace fsb {
namespace core {
    class World;
}
}

namespace fsb {
namespace entities {

enum class EntityState {
    IDLE,
    MOVING
};

class BaseEntity {
protected:
    int id_;
    int x_;
    int y_;

    // FSM 状态数据
    EntityState state_;
    int state_ticks_; // 记录在当前状态下已经持续了多少个 Tick (时间跨度)

public:
    BaseEntity(int id, int x, int y) 
        : id_(id), x_(x), y_(y), state_(EntityState::IDLE), state_ticks_(0) {}

    virtual ~BaseEntity() = default;

    virtual void update(core::World& world) = 0;

    // --- 基础 Getter 与 Setter ---
    int getId() const { return id_; }
    int getX() const { return x_; }
    int getY() const { return y_; }
    
    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
    }

    // --- 状态机 Getter 与 Setter ---
    EntityState getState() const { return state_; }
    void setState(EntityState state) { state_ = state; }
    
    int getStateTicks() const { return state_ticks_; }
    void setStateTicks(int ticks) { state_ticks_ = ticks; }
    
    // 状态自增（供 Policy 在每次 update 时调用）
    void incrementStateTicks() { state_ticks_++; }

    // 辅助函数：将内部枚举转换为字符串，方便 World 导出 JSON 给前端
    std::string getStateAsString() const {
        switch (state_) {
            case EntityState::IDLE:   return "idle";
            case EntityState::MOVING: return "moving";
            default:                  return "unknown";
        }
    }
};

} // namespace entities
} // namespace fsb
