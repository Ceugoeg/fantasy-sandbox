#pragma once

#include <cstdlib> // 提供 std::rand()
#include "entities/BaseEntity.h"
#include "core/World.h"
#include "events/GameEvents.h"

namespace fsb {
namespace entities {
namespace policies {

class RandomWalkPolicy {
private:
    static constexpr int IDLE_DURATION = 4;
    static constexpr int MOVE_DURATION = 3;
public:
    // 策略的核心执行接口。
    // Entity 模板在调用时，会将自身 (*this) 和世界 (world) 传递进来。
    void execute(BaseEntity& entity, core::World& world) {
        entity.incrementStateTicks();

        EntityState current_state = entity.getState();
        int current_ticks = entity.getStateTicks();

        switch (current_state) {
            case EntityState::IDLE: {
                if (current_ticks >= IDLE_DURATION) {
                    entity.setState(EntityState::MOVING);
                    entity.setStateTicks(0);

                    // 拆分写法：先显式实例化事件对象，再发布，治好 IDE 的晕车
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
                int dx = (std::rand() % 3) - 1;
                int dy = (std::rand() % 3) - 1;
                int new_x = entity.getX() + dx;
                int new_y = entity.getY() + dy;

                if (world.getTerrainAt(new_x, new_y) == 0) {
                    int old_x = entity.getX();
                    int old_y = entity.getY();
                    entity.setPosition(new_x, new_y);

                    if (old_x != new_x || old_y != new_y) {
                        // 拆分写法
                        fsb::events::EntityMovedEvent event{
                            entity.getId(),
                            old_x, old_y,
                            new_x, new_y
                        };
                        world.getEventBus().publish(event);
                    }
                }

                if (current_ticks >= MOVE_DURATION) {
                    entity.setState(EntityState::IDLE);
                    entity.setStateTicks(0);

                    // 拆分写法
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