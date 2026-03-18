#pragma once

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <any>

namespace fsb {
namespace core {

class EventBus {
private:
    // 核心：Type Erasure
    // 抹除具体事件类型的回调函数签名：统一接收一个 std::any 包装的事件
    using EventHandler = std::function<void(const std::any&)>;

    // 神经网容器：Key 是事件类型在编译期的唯一哈希标识，Value 是该事件对应的所有回调函数列表
    std::unordered_map<std::type_index, std::vector<EventHandler>> subscribers_;

public:
    EventBus() = default;
    ~EventBus() = default;

    // --- 订阅接口 ---
    // 外界传入一个强类型的回调函数，例如：[](const EntityMovedEvent& e) { ... }
    template <typename EventType>
    void subscribe(std::function<void(const EventType&)> callback) {
        // 获取类型的唯一标识
        std::type_index type_idx(typeid(EventType));

        // 将强类型回调包装成接受 std::any 的泛型回调
        EventHandler wrapper = [callback](const std::any& event_data) {
            // 安全地将 std::any 拆包，强转回 EventType，并喂给真正的回调函数
            callback(std::any_cast<const EventType&>(event_data));
        };

        // 挂载到神经网上
        subscribers_[type_idx].push_back(wrapper);
    }

    // --- 发布接口 ---
    // 外界扔进来一个具体的事件结构体，例如：publish(EntityMovedEvent{1, 0, 0, 1, 1});
    template <typename EventType>
    void publish(const EventType& event) {
        std::type_index type_idx(typeid(EventType));

        // 如果有人订阅了这个事件，就依次触发他们的回调
        if (subscribers_.find(type_idx) != subscribers_.end()) {
            for (const auto& handler : subscribers_[type_idx]) {
                // 将具体事件作为实参传入，C++ 会自动将其隐式转换为 std::any 塞给包装器
                handler(event);
            }
        }
    }
};

} // namespace core
} // namespace fsb
