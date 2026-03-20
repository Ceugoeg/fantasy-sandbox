#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <sstream>

#include "core/World.h"
#include "entities/EntityFactory.h"

std::atomic<bool> is_running(true);
std::mutex world_mutex;

void commandListener(fsb::core::World& world) {
    std::string line;
    while (is_running && std::getline(std::cin, line)) {
        if (line == "QUIT" || line == "EXIT") {
            is_running = false;
            break;
        }
        
        // 使用标准的 istringstream 进行极其优雅且健壮的指令解析
        if (line.rfind("SPAWN", 0) == 0) {
            std::istringstream iss(line);
            std::string cmd, type;
            int x = 0, y = 0;

            // 格式期望: SPAWN <type> <x> <y>
            iss >> cmd >> type >> x >> y;

            if (!iss.fail()) {
                std::lock_guard<std::mutex> lock(world_mutex);
                static int entity_id_counter = 1000;
                
                // 将创造实体的重任全权委托给 EntityFactory
                auto entity = fsb::entities::EntityFactory::create(
                    type, entity_id_counter++, x, y, world
                );
                
                world.addEntity(entity);
                std::cerr << "[Oracle] Miracle executed: SPAWN " << type 
                          << " at (" << x << ", " << y << ")" << std::endl;
            } else {
                std::cerr << "[Oracle] Invalid SPAWN format. Use: SPAWN <type> <x> <y>" << std::endl;
            }
        }
    }
}

int main() {
    std::cerr << "[Engine] Booting up..." << std::endl;

    // 创世解析
    fsb::core::World world("assets/maps/temp2.json");

    std::thread listener_thread(commandListener, std::ref(world));

    // 游戏主循环 (Tick Loop)
    const int TARGET_TPS = 10;
    const auto tick_duration = std::chrono::milliseconds(1000 / TARGET_TPS);

    while (is_running) {
        auto tick_start = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(world_mutex);
            world.tick();
            
            // 将包含生物真实 type (用于多态渲染) 和 state 的状态输出给 Node 网关
            std::cout << world.exportStateAsJson() << std::endl;
        }

        auto tick_end = std::chrono::steady_clock::now();
        auto elapsed = tick_end - tick_start;

        if (elapsed < tick_duration) {
            std::this_thread::sleep_for(tick_duration - elapsed);
        }
    }

    is_running = false;
    if (listener_thread.joinable()) {
        listener_thread.join();
    }

    std::cerr << "[Engine] Shutting down gracefully." << std::endl;
    return 0;
}