#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <optional>
#include <sstream>

#include "core/Logger.h"

namespace fsb {
namespace core {

// ============================================================================
// 指令意图类型 (Intent Types)
// ============================================================================
// 所有从外部（Node.js 网关 → stdin）传入的操作，必须先被解析为结构化的 Intent，
// 经由主循环统一消费与校验，绝不允许绕过仲裁直接修改实体属性。
// 这是"零信任"架构在引擎内部的第一道防线。

enum class IntentType {
    SPAWN,          // 创造实体
    QUIT,           // 引擎关闭
    UNKNOWN         // 无法识别的指令（将被静默丢弃并记录日志）
};

// 结构化意图载体：携带指令类型与解析后的参数
struct Intent {
    IntentType type = IntentType::UNKNOWN;
    std::string entity_type;    // SPAWN 专用：生物种类
    int x = 0;                  // SPAWN 专用：坐标 X
    int y = 0;                  // SPAWN 专用：坐标 Y
};

// ============================================================================
// 指令仲裁队列 (Command/Intent Queue)
// ============================================================================
// 线程安全的 MPSC 队列（多生产者：I/O 线程；单消费者：主循环）。
// I/O 线程解析原始文本为 Intent 后压入，主循环在 tick 前统一消费。

class CommandQueue {
private:
    std::queue<Intent> queue_;
    mutable std::mutex mutex_;

public:
    CommandQueue() = default;
    ~CommandQueue() = default;

    // --- 静态解析器 ---
    // 将原始 stdin 行文本解析为结构化 Intent。
    // 解析职责与队列职责分离，便于未来扩展 JSON 格式指令。
    static Intent parse(const std::string& raw_line) {
        Intent intent;

        if (raw_line == "QUIT" || raw_line == "EXIT") {
            intent.type = IntentType::QUIT;
            return intent;
        }

        if (raw_line.rfind("SPAWN", 0) == 0) {
            std::istringstream iss(raw_line);
            std::string cmd;
            // 格式期望: SPAWN <type> <x> <y>
            iss >> cmd >> intent.entity_type >> intent.x >> intent.y;

            if (!iss.fail() && !intent.entity_type.empty()) {
                intent.type = IntentType::SPAWN;
            } else {
                LOG_WARN("Malformed SPAWN intent: '%s'. Expected: SPAWN <type> <x> <y>", raw_line.c_str());
                intent.type = IntentType::UNKNOWN;
            }
            return intent;
        }

        // 兜底：无法识别的指令
        LOG_WARN("Unrecognized command dropped: '%s'", raw_line.c_str());
        intent.type = IntentType::UNKNOWN;
        return intent;
    }

    // --- 生产者接口 ---
    // 供独立的 I/O 线程调用。将已解析的 Intent 压入队列。
    // RAII 风格加锁，离开作用域自动解锁，避免死锁或异常导致的锁泄漏。
    void push(const Intent& intent) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(intent);
    }

    // --- 消费者接口 ---
    // 供主循环 (World) 高频调用，绝对非阻塞。
    // 返回 std::nullopt 明确表达"没有待处理的意图"的语义。
    std::optional<Intent> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }

        Intent intent = queue_.front();
        queue_.pop();
        return intent;
    }
};

} // namespace core
} // namespace fsb
