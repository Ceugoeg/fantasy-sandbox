#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// 引入我们自己编写的核心头文件
#include "core/World.h"
#include "entities/Entity.h"
#include "entities/policies/RandomWalkPolicy.h"

// 为了代码简洁，使用命名空间
using namespace fsb::core;
using namespace fsb::entities;
using namespace fsb::entities::policies;

int main() {
    std::cout << "--- 异世界 (Fantasy Sandbox) 创世引擎启动 ---" << std::endl;

    // 1. 创世：开辟一个 10x10 的小型测试沙盘
    World world(10, 10);

    // 2. 捏造生物：使用 std::make_shared 在堆内存中安全地创建实体。
    // 这里我们传入了 RandomWalkPolicy，赋予它们随机乱走的“灵魂”。
    // 参数：ID, 初始X坐标, 初始Y坐标
    auto slime_a = std::make_shared<Entity<RandomWalkPolicy>>(1, 5, 5); 
    auto slime_b = std::make_shared<Entity<RandomWalkPolicy>>(2, 8, 2);

    // 将生物投入世界
    world.addEntity(slime_a);
    world.addEntity(slime_b);

    std::cout << "【初始状态】:\n" << world.exportStateAsJson() << "\n\n";

    // 3. 时间流逝：模拟 5 帧 (Tick) 的世界演化
    for (int i = 1; i <= 5; ++i) {
        std::cout << ">>> 第 " << i << " 纪元 (Tick) 推演中..." << std::endl;
        
        // 核心驱动：调用这一句，world 就会遍历所有实体并调用它们的 policy
        world.tick(); 
        
        // 导出当前世界状态（未来这一步会将 JSON 发送给 Node.js）
        std::cout << world.exportStateAsJson() << "\n\n";
        
        // 为了方便我们在终端里看清输出，让当前线程休眠 500 毫秒。
        // 在真实的游戏引擎中，这往往会被替换为精确的帧率控制 (FPS) 逻辑。
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "--- 局部演化测试结束 ---" << std::endl;
    return 0;
}
