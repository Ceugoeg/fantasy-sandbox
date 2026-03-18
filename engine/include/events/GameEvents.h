#pragma once
    
#include <string>
#include "../entities/BaseEntity.h"  // 用到 EntityState 枚举

namespace fsb {
namespace events{
// 1. 实体状态改变
struct EntityStateChangedEvent {
    int entity_id;
    fsb::entities::EntityState old_state;
    fsb::entities::EntityState new_state;
};

// 2. 实体移动
struct EntityMovedEvent {
    int entity_id;
    int from_x;
    int from_y;
    int to_x;
    int to_y;
};

// 3. 系统通知
struct SystemMessageEvent {
    std::string message;
};

// 4. LLM 预留接口
struct LLMCommandEvent {
    std::string command_json;
};

} // namespace event
} // namespace fsb