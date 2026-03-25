#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>

#include "core/World.h"
#include "core/CommandQueue.h"
#include "core/Logger.h"
#include "entities/EntityFactory.h"

// ============================================================================
// 全局状态
// ============================================================================
std::atomic<bool> is_running(true);

// 指令仲裁队列：I/O 线程与主循环之间唯一的通信桥梁。
// 主循环不再持有 world_mutex，因为只有主循环访问 World。
fsb::core::CommandQueue command_queue;

// ============================================================================
// I/O 监听线程
// ============================================================================
// 职责：从 stdin 逐行读取 Node.js 网关转发的指令，
//        解析为结构化 Intent 后压入仲裁队列。
//        绝不直接修改 World 或任何实体属性。
void commandListener() {
    std::string line;
    while (is_running && std::getline(std::cin, line)) {
        // 解析原始文本为结构化 Intent
        fsb::core::Intent intent = fsb::core::CommandQueue::parse(line);

        // QUIT 特殊处理：在生产端直接翻转原子标志，确保主循环最快响应
        if (intent.type == fsb::core::IntentType::QUIT) {
            is_running = false;
            // 仍然压入队列，让 World::processIntents 有机会做清理日志
            command_queue.push(intent);
            break;
        }

        // 非 UNKNOWN 的有效 Intent 入队等待主循环消费
        if (intent.type != fsb::core::IntentType::UNKNOWN) {
            command_queue.push(intent);
        }
    }
}

// ============================================================================
// 引擎入口
// ============================================================================
int main() {
    LOG_INFO("Engine booting up...");

    // 创世解析
    fsb::core::World world("assets/maps/temp2.json");

    // 启动独立的 I/O 监听线程（不再传入 World 引用，彻底解耦）
    std::thread listener_thread(commandListener);

    // ========================================================================
    // 游戏主循环 (Fixed Timestep Tick Loop)
    // ========================================================================
    // 基于 std::chrono 的固定时间步长循环。
    // 每帧严格按照 TARGET_TPS 节拍运行，多余时间交给 sleep_for 消化。
    const int TARGET_TPS = 10;
    const auto tick_duration = std::chrono::milliseconds(1000 / TARGET_TPS);

    while (is_running) {
        auto tick_start = std::chrono::steady_clock::now();

        // 1. 仲裁阶段：消费 Intent 队列，校验并执行
        world.processIntents(command_queue);

        // 2. 物理阶段：驱动所有实体的 AI / FSM / 移动
        world.tick();

        // 3. 数据导出阶段：序列化为单行 NDJSON，强制冲刷 stdout
        //    stdout 是与 Node.js 网关之间的神圣数据通道，
        //    每帧输出严格遵循：一行 JSON + '\n' + fflush。
        nlohmann::json snapshot = world.exportStateAsJson();
        std::string json_str = snapshot.dump();

        // printf + fflush 组合拳，确保跨进程 pipe 无缓冲延迟、无粘包
        printf("%s\n", json_str.c_str());
        fflush(stdout);

        // 4. 节拍校准：精确补偿本帧耗时，维持恒定 TPS
        auto tick_end = std::chrono::steady_clock::now();
        auto elapsed = tick_end - tick_start;

        if (elapsed < tick_duration) {
            std::this_thread::sleep_for(tick_duration - elapsed);
        }
    }

    // ========================================================================
    // 优雅关机
    // ========================================================================
    is_running = false;
    if (listener_thread.joinable()) {
        listener_thread.join();
    }

    LOG_INFO("Engine shutting down gracefully.");
    return 0;
}