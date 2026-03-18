#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <atomic>

// 引入自己编写的核心头文件
#include "core/World.h"
#include "core/CommandQueue.h"
#include "entities/Entity.h"
#include "entities/policies/RandomWalkPolicy.h"
#include "events/GameEvents.h"

using namespace fsb::core;
using namespace fsb::entities;
using namespace fsb::entities::policies;

// 全局 UUID 生成器，保证每个实体拥有绝对唯一的身份标识
std::atomic<int> next_entity_id{1};

// 后台 I/O 监听线程的工作函数
void ioWorker(CommandQueue& cmd_queue) {
    std::string line;
    // 阻塞式读取 stdin，一旦终端有输入并按下回车，就将其塞入安全的指令队列
    while (std::getline(std::cin, line)) {
        if (!line.empty()) {
            cmd_queue.push(line);
        }
    }
}

int main() {
    World world(50, 50);
    CommandQueue cmd_queue;

    // 【启动 I/O 侦听线程】
    // 使用 std::thread 开启独立线程，并立刻 detach 让其在后台默默运行，不阻塞主线程
    std::thread(ioWorker, std::ref(cmd_queue)).detach();

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

    // 捏造初始生物：使用原子自增 ID 和语义化类型名
    world.addEntity(std::make_shared<Entity<RandomWalkPolicy>>(next_entity_id++, "slime", 25, 25)); 
    world.addEntity(std::make_shared<Entity<RandomWalkPolicy>>(next_entity_id++, "slime", 28, 22));

    // 时间流逝 (主循环)
    while (true) {
        // 1. 【非阻塞消费指令】：处理队列中积压的所有神谕
        while (auto cmd_opt = cmd_queue.tryPop()) {
            std::string cmd_str = cmd_opt.value();
            std::stringstream ss(cmd_str);
            std::string action;
            ss >> action;

            // 极简的神谕解析器 (Factory 雏形)
            if (action == "SPAWN") {
                std::string type;
                int x, y;
                // 尝试解析剩余参数
                if (ss >> type >> x >> y) {
                    int new_id = next_entity_id++;
                    if (type == "slime") {
                        world.addEntity(std::make_shared<Entity<RandomWalkPolicy>>(new_id, type, x, y));
                        std::cerr << "[Oracle] Spawned " << type << " at (" << x << ", " << y << ") with ID " << new_id << std::endl;
                    } else {
                        std::cerr << "[Oracle] Unknown entity type: " << type << std::endl;
                    }
                } else {
                    std::cerr << "[Oracle] SPAWN command syntax error. Usage: SPAWN <type> <x> <y>" << std::endl;
                }
            }
        }

        // 2. 【世界运转】
        world.tick();
        
        // 3. 【状态导出】
        std::cout << world.exportStateAsJson() << std::endl;
        
        // 4. 【帧率控制】
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
