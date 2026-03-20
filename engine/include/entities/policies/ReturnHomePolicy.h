#pragma once

#include <vector>
#include <utility>
#include "entities/policies/IPolicy.h"
#include "entities/BaseEntity.h"
#include "core/World.h"
#include "core/Pathfinder.h"
#include "events/GameEvents.h"

namespace fsb {
namespace entities {
namespace policies {

class ReturnHomePolicy : public IPolicy {
private:
    std::vector<std::pair<int, int>> path_;
    size_t current_path_index_ = 0;

public:
    void onEnter(BaseEntity& entity, core::World& world) override {
        int biome_min = entity.getBiomeMin();
        int biome_max = entity.getBiomeMax();

        if (biome_min == -1 || biome_max == -1) return;

        // 1. 雷达扫描：寻找离当前坐标最近的偏好群落图块
        auto target = core::Pathfinder::findNearestBiome(
            world, entity.getX(), entity.getY(), biome_min, biome_max
        );

        if (target.first != -1 && target.second != -1) {
            // 2. 启发式寻路：计算回家的确切路径
            path_ = core::Pathfinder::findPath(
                world, entity.getX(), entity.getY(), target.first, target.second
            );
        }
        current_path_index_ = 0;
    }

    void execute(BaseEntity& entity, core::World& world) override {
        // 如果扫描不到家，或者被包围没有路径，亦或是已经走完路径
        if (path_.empty() || current_path_index_ >= path_.size()) {
            entity.popPolicy(world); // 销毁自我，交还控制权
            return; // ⚠️ 必须立刻 return，因为 popPolicy 后 this 指针已失效
        }

        entity.incrementStateTicks();
        EntityState current_state = entity.getState();
        int current_ticks = entity.getStateTicks();

        if (current_state == EntityState::IDLE) {
            int target_ticks = entity.getTargetStateTicks();
            if (target_ticks == 0) {
                // 逃命时心急如焚，发呆的时间比正常游荡短
                entity.setTargetStateTicks(2); 
                target_ticks = 2;
            }

            if (current_ticks >= target_ticks) {
                entity.setState(EntityState::MOVING);
                entity.setStateTicks(0);
                entity.setTargetStateTicks(3); // 逃生的步伐频率更高

                fsb::events::EntityStateChangedEvent event{
                    entity.getId(), EntityState::IDLE, EntityState::MOVING
                };
                world.getEventBus().publish(event);
            }
        } 
        else if (current_state == EntityState::MOVING) {
            // 在 Moving 状态的第一帧执行真实坐标移动
            if (current_ticks == 1) {
                int next_x = path_[current_path_index_].first;
                int next_y = path_[current_path_index_].second;

                if (!world.isBlocked(next_x, next_y)) {
                    int old_x = entity.getX();
                    int old_y = entity.getY();
                    entity.setPosition(next_x, next_y);
                    current_path_index_++; // 推进到下一个路径节点

                    fsb::events::EntityMovedEvent event{
                        entity.getId(), old_x, old_y, next_x, next_y
                    };
                    world.getEventBus().publish(event);
                } else {
                    // 发生了意外：原本计划好的路上突然出现了动态阻挡（防患于未然）
                    // 直接放弃当前旧路径，销毁自己。下个 Tick 底层游荡策略会重新触发回家逻辑。
                    entity.popPolicy(world);
                    return; 
                }
            }

            // 等待前端动画播放完毕
            if (current_ticks >= entity.getTargetStateTicks()) {
                entity.setState(EntityState::IDLE);
                entity.setStateTicks(0);
                entity.setTargetStateTicks(2);

                fsb::events::EntityStateChangedEvent event{
                    entity.getId(), EntityState::MOVING, EntityState::IDLE
                };
                world.getEventBus().publish(event);

                // 【核心生态判定】：每走完一格，立刻检查是否已经脱离险境
                int current_biome = world.getTerrainAt(entity.getX(), entity.getY());
                if (current_biome >= entity.getBiomeMin() && current_biome <= entity.getBiomeMax()) {
                    entity.popPolicy(world); // 成功脱险，销毁自我
                    return;
                }
            }
        }
    }
};

} // namespace policies
} // namespace entities
} // namespace fsb