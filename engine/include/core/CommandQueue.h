#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <optional>

namespace fsb {
namespace core {

class CommandQueue {
private:
    std::queue<std::string> queue_;
    mutable std::mutex mutex_;

public:
    CommandQueue() = default;
    ~CommandQueue() = default;

    // --- 生产者接口 ---
    // 供独立的 I/O 线程调用。当接收到 Node.js 发来的 stdin 字符串时，
    // 获取锁并将其压入队列。
    void push(const std::string& command) {
        // RAII 风格加锁，离开作用域自动解锁，避免死锁或异常导致的锁泄漏
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(command);
    }

    // --- 消费者接口 ---
    // 供主循环 (World) 高频调用，绝对非阻塞。
    std::optional<std::string> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt; // 明确表达“没有指令”的语义
        }
        
        std::string cmd = queue_.front();
        queue_.pop();
        return cmd;
    }
};

} // namespace core
} // namespace fsb

