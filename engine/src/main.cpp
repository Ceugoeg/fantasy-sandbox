#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// 引入自己编写的核心头文件
#include "core/World.h"
#include "entities/Entity.h"
#include "entities/policies/RandomWalkPolicy.h"
#include "events/GameEvents.h"

// 为了代码简洁，使用命名空间
using namespace fsb::core;
using namespace fsb::entities;
using namespace fsb::entities::policies;

int main() {

    World world(50, 50);

    // 【事件订阅】：挂载对状态改变的监听
    world.getEventBus().subscribe<fsb::events::EntityStateChangedEvent>(
        [](const fsb::events::EntityStateChangedEvent& e) {
            // 使用 cerr 隔离数据流，不干扰发给前端的 JSON
            std::cerr << "[EventBus] Entity " << e.entity_id 
                      << " changed state to " 
                      << (e.new_state == EntityState::MOVING ? "MOVING" : "IDLE") 
                      << std::endl;
        }
    );

    // 【事件订阅】：挂载对位置移动的监听
    world.getEventBus().subscribe<fsb::events::EntityMovedEvent>(
        [](const fsb::events::EntityMovedEvent& e) {
            std::cerr << "[EventBus] Entity " << e.entity_id 
                      << " moved from (" << e.from_x << ", " << e.from_y << ") "
                      << "to (" << e.to_x << ", " << e.to_y << ")" 
                      << std::endl;
        }
    );

    // 捏造生物：使用 std::make_shared 在堆内存中安全地创建实体。
    auto slime_a = std::make_shared<Entity<RandomWalkPolicy>>(1, 25, 25); 
    auto slime_b = std::make_shared<Entity<RandomWalkPolicy>>(2, 28, 22);

    // 将生物投入世界
    world.addEntity(slime_a);
    world.addEntity(slime_b);

    // 时间流逝
    while (true) {
        world.tick();
        std::cout << world.exportStateAsJson() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
