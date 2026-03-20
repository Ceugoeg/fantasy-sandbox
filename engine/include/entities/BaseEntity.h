#pragma once

#include <string>
#include <vector>
#include <memory>

#include "entities/policies/IPolicy.h"

// 前向声明 World，因为 BaseEntity 只需要知道 World 是个引用，不用调用 World 的方法
namespace fsb {
namespace core {
    class World;
}
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

    // --- FSM 动画与刻状态 ---
    EntityState state_;
    int state_ticks_;         
    int target_state_ticks_;  

    // --- AI 策略栈 (下推自动机) ---
    std::vector<std::shared_ptr<policies::IPolicy>> policy_stack_;

    // --- 生物偏好 (Biome Constraints) ---
    // 默认 [-1, -1] 表示无视地形限制 (全图乱跑)
    int biome_min_;
    int biome_max_;

public:
    BaseEntity(int id, const std::string& type_name, int x, int y) 
        : id_(id), type_name_(type_name), x_(x), y_(y), 
          state_(EntityState::IDLE), state_ticks_(0), target_state_ticks_(0),
          biome_min_(-1), biome_max_(-1) {}

    virtual ~BaseEntity() = default;

    // 基础 Update 逻辑：永远只执行栈顶策略
    virtual void update(core::World& world) {
        if (!policy_stack_.empty()) {
            policy_stack_.back()->execute(*this, world);
        }
    }

    // --- 策略栈控制 API ---
    void pushPolicy(std::shared_ptr<policies::IPolicy> policy, core::World& world) {
        if (!policy_stack_.empty()) {
            policy_stack_.back()->onExit(*this, world); // 挂起旧策略
        }
        policy_stack_.push_back(policy);
        policy->onEnter(*this, world); // 激活新策略
    }

    void popPolicy(core::World& world) {
        if (!policy_stack_.empty()) {
            policy_stack_.back()->onExit(*this, world); // 彻底销毁当前策略
            policy_stack_.pop_back();
            if (!policy_stack_.empty()) {
                policy_stack_.back()->onEnter(*this, world); // 唤醒下层策略
            }
        }
    }

    void clearPolicies(core::World& world) {
        while (!policy_stack_.empty()) {
            popPolicy(world);
        }
    }

    // --- 生态偏好 Getter / Setter ---
    void setBiomePreference(int min_id, int max_id) {
        biome_min_ = min_id;
        biome_max_ = max_id;
    }
    int getBiomeMin() const { return biome_min_; }
    int getBiomeMax() const { return biome_max_; }

    // --- 基础 Getter 与 Setter ---
    int getId() const { return id_; }
    const std::string& getTypeName() const { return type_name_; }
    int getX() const { return x_; }
    int getY() const { return y_; }
    
    void setPosition(int x, int y) {
        x_ = x;
        y_ = y;
    }

    // --- FSM 状态机 Getter 与 Setter ---
    EntityState getState() const { return state_; }
    void setState(EntityState state) { state_ = state; }
    
    int getStateTicks() const { return state_ticks_; }
    void setStateTicks(int ticks) { state_ticks_ = ticks; }

    int getTargetStateTicks() const { return target_state_ticks_; }
    void setTargetStateTicks(int ticks) { target_state_ticks_ = ticks; }
    
    void incrementStateTicks() { state_ticks_++; }

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