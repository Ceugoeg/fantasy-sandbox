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
    std::string type_name_;
    int x_;
    int y_;

    // FSM 状态数据
    EntityState state_;
    int state_ticks_;         // 当前状态已持续 Tick 数
    int target_state_ticks_;  // 目标持续 Tick 数

public:
    // 注意这里的参数顺序，已经修正为与 Entity.h 对应的 id, type_name, x, y
    // 并且冒号后面的初始化顺序完全匹配变量的声明顺序
    BaseEntity(int id, const std::string& type_name, int x, int y) 
        : id_(id), type_name_(type_name), x_(x), y_(y), 
          state_(EntityState::IDLE), state_ticks_(0), target_state_ticks_(0) {}

    virtual ~BaseEntity() = default;

    virtual void update(core::World& world) = 0;

    // --- 基础 Getter 与 Setter ---
    int getId() const { return id_; }
    const std::string& getTypeName() const { return type_name_; }
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

    int getTargetStateTicks() const { return target_state_ticks_; }
    void setTargetStateTicks(int ticks) { target_state_ticks_ = ticks; }
    
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