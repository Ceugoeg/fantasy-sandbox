#pragma once

#include <random>
#include "entities/BaseEntity.h"
#include "core/World.h"
#include "events/GameEvents.h"

namespace fsb {
namespace entities {
namespace policies {

class RandomWalkPolicy {
private:
    inline static std::mt19937 rng{std::random_device{}()};
    
    inline static std::uniform_int_distribution<int> idle_dist{3, 8};
    // 把移动状态的持续时间稍微缩短一点，因为我们只走 1 格了
    inline static std::uniform_int_distribution<int> move_dist{2, 4}; 
    inline static std::uniform_int_distribution<int> dir_dist{-1, 1}; 

    int current_dx_ = 0;
    int current_dy_ = 0;

public:
    void execute(BaseEntity& entity, core::World& world) {
        entity.incrementStateTicks();

        EntityState current_state = entity.getState();
        int current_ticks = entity.getStateTicks();
        int target_ticks = entity.getTargetStateTicks();

        if (target_ticks == 0) {
            target_ticks = idle_dist(rng);
            entity.setTargetStateTicks(target_ticks);
        }

        switch (current_state) {
            case EntityState::IDLE: {
                if (current_ticks >= target_ticks) {
                    entity.setState(EntityState::MOVING);
                    entity.setStateTicks(0);
                    entity.setTargetStateTicks(move_dist(rng));

                    current_dx_ = dir_dist(rng);
                    current_dy_ = dir_dist(rng);

                    fsb::events::EntityStateChangedEvent event{
                        entity.getId(),
                        EntityState::IDLE,
                        EntityState::MOVING
                    };
                    world.getEventBus().publish(event);
                }
                break;
            }
            case EntityState::MOVING: {
                // 【核心修正】：整趟 MOVING 旅程，我们只在第 1 帧迈出唯一的一步
                if (current_ticks == 1) {
                    if (current_dx_ != 0 || current_dy_ != 0) {
                        int new_x = entity.getX() + current_dx_;
                        int new_y = entity.getY() + current_dy_;

                        if (world.getTerrainAt(new_x, new_y) == 0) {
                            int old_x = entity.getX();
                            int old_y = entity.getY();
                            entity.setPosition(new_x, new_y);

                            fsb::events::EntityMovedEvent event{
                                entity.getId(),
                                old_x, old_y,
                                new_x, new_y
                            };
                            world.getEventBus().publish(event);
                        } else {
                            // 撞墙，取消后续的拖延时间，直接准备停下
                            entity.setTargetStateTicks(current_ticks);
                        }
                    }
                }

                // 第 2、3、4 帧会直接跳过上面的 if，什么都不做，纯粹为了让前端把这 1 格动画播完
                if (current_ticks >= target_ticks) {
                    entity.setState(EntityState::IDLE);
                    entity.setStateTicks(0);
                    entity.setTargetStateTicks(idle_dist(rng));

                    fsb::events::EntityStateChangedEvent event{
                        entity.getId(),
                        EntityState::MOVING,
                        EntityState::IDLE
                    };
                    world.getEventBus().publish(event);
                }
                break;
            }
        }
    }
};

} // namespace policies
} // namespace entities
} // namespace fsb