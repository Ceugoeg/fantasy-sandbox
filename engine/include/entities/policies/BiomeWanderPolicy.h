#pragma once

#include <random>
#include <memory>
#include "entities/policies/IPolicy.h"
#include "entities/policies/ReturnHomePolicy.h"
#include "entities/BaseEntity.h"
#include "core/World.h"
#include "events/GameEvents.h"

namespace fsb {
namespace entities {
namespace policies {

class BiomeWanderPolicy : public IPolicy {
private:
    inline static std::mt19937 rng{std::random_device{}()};
    
    // 【时间尺度校准】 1 Tick = 0.1 秒
    // 发呆 20~50 刻 (即 2.0秒 ~ 5.0秒)，符合生物的慵懒天性
    inline static std::uniform_int_distribution<int> idle_dist{20, 50}; 
    // 移动 6~10 刻 (即 0.6秒 ~ 1.0秒)，给予前端充足的补帧和动画播放时间
    inline static std::uniform_int_distribution<int> move_dist{6, 10};  

    // 纯正的四向移动，杜绝原地抽搐
    inline static const int DX[4] = {0, 0, -1, 1};
    inline static const int DY[4] = {-1, 1, 0, 0};
    inline static std::uniform_int_distribution<int> dir_dist{0, 3};

    int current_dx_ = 0;
    int current_dy_ = 0;

public:
    void onEnter(BaseEntity& entity, core::World& /*world*/) override {
        entity.setState(EntityState::IDLE);
        entity.setStateTicks(0);
        entity.setTargetStateTicks(idle_dist(rng));
    }

    void execute(BaseEntity& entity, core::World& world) override {
        int biome_min = entity.getBiomeMin();
        int biome_max = entity.getBiomeMax();

        if (biome_min != -1 && biome_max != -1) {
            int current_biome = world.getTerrainAt(entity.getX(), entity.getY());
            if (current_biome < biome_min || current_biome > biome_max) {
                // 异乡告警，交出控制权
                entity.pushPolicy(std::make_shared<ReturnHomePolicy>(), world);
                return; 
            }
        }

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

                    int dir_idx = dir_dist(rng);
                    current_dx_ = DX[dir_idx];
                    current_dy_ = DY[dir_idx];

                    fsb::events::EntityStateChangedEvent event{
                        entity.getId(), EntityState::IDLE, EntityState::MOVING
                    };
                    world.getEventBus().publish(event);
                }
                break;
            }
            case EntityState::MOVING: {
                // 第一刻瞬间完成绝对坐标的变更，剩下的时间全留给前端去插值平滑
                if (current_ticks == 1) {
                    if (current_dx_ != 0 || current_dy_ != 0) {
                        int new_x = entity.getX() + current_dx_;
                        int new_y = entity.getY() + current_dy_;

                        bool is_biome_valid = true;
                        if (biome_min != -1 && biome_max != -1) {
                            int next_biome = world.getTerrainAt(new_x, new_y);
                            if (next_biome < biome_min || next_biome > biome_max) {
                                is_biome_valid = false;
                            }
                        }

                        if (!world.isBlocked(new_x, new_y) && is_biome_valid) {
                            int old_x = entity.getX();
                            int old_y = entity.getY();
                            entity.setPosition(new_x, new_y);

                            fsb::events::EntityMovedEvent event{
                                entity.getId(), old_x, old_y, new_x, new_y
                            };
                            world.getEventBus().publish(event);
                        } else {
                            // 撞墙时不重置 ticks，强行把分配的 0.6~1 秒“发呆”发完
                            // 这样前端不会因为状态瞬间切回 IDLE 而发生动画断档
                        }
                    }
                }

                if (current_ticks >= target_ticks) {
                    entity.setState(EntityState::IDLE);
                    entity.setStateTicks(0);
                    entity.setTargetStateTicks(idle_dist(rng));

                    fsb::events::EntityStateChangedEvent event{
                        entity.getId(), EntityState::MOVING, EntityState::IDLE
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