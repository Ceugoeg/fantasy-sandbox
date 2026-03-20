#pragma once

#include <string>
#include <memory>
#include "entities/BaseEntity.h"
#include "entities/policies/BiomeWanderPolicy.h"
#include "core/World.h"

namespace fsb {
namespace entities {

class EntityFactory {
public:
    // 根据生物名称、坐标动态组装实体，并准备注入世界
    static std::shared_ptr<BaseEntity> create(
        const std::string& type_name, 
        int id, 
        int x, 
        int y, 
        core::World& world) 
    {
        // 1. 实例化通用的物理躯壳
        auto entity = std::make_shared<BaseEntity>(id, type_name, x, y);

        // 2. 根据物种类别，注入灵魂（生态群落偏好与初始策略）
        if (type_name == "green_slime") {
            // 平原史莱姆：偏好草地 (包含交错的森林)
            entity->setBiomePreference(154, 291); 
            // 压入底层的游荡逻辑
            entity->pushPolicy(std::make_shared<policies::BiomeWanderPolicy>(), world);
        } 
        else if (type_name == "orange_slime") {
            // 沙漠史莱姆：偏好沙地
            entity->setBiomePreference(0, 137);
            entity->pushPolicy(std::make_shared<policies::BiomeWanderPolicy>(), world);
        }
        else {
            // 默认兜底/神谕生物：无视任何地形限制，全图乱跑
            entity->setBiomePreference(-1, -1);
            entity->pushPolicy(std::make_shared<policies::BiomeWanderPolicy>(), world);
        }

        return entity;
    }
};

} // namespace entities
} // namespace fsb