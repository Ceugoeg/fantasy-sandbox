#pragma once

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <any>
#include <algorithm>

namespace fsb {
namespace core {

// ============================================================================
// 事件总线 (EventBus) — 生命周期安全版
// ============================================================================
// 核心改造：每个订阅者必须提供一个唯一的 subscriber_id (通常为 EntityID)。
// 当实体被销毁时，可通过 unsubscribe 按 ID 注销回调，彻底杜绝悬垂指针。
//
// 设计铁律：回调函数禁止捕获 `this` 裸指针。
// 正确做法是捕获 EntityID（值拷贝），运行时通过 World 查找实体是否存活。
// ============================================================================

class EventBus {
private:
    // Type Erasure：抹除具体事件类型的回调签名
    using EventHandler = std::function<void(const std::any&)>;

    // 订阅记录：将 subscriber_id 与回调函数绑定，支持按 ID 精确注销
    struct Subscription {
        int subscriber_id;
        EventHandler handler;
    };

    // 神经网容器：Key 是事件类型在编译期的唯一哈希标识，
    // Value 是该事件对应的所有订阅记录列表
    std::unordered_map<std::type_index, std::vector<Subscription>> subscribers_;

public:
    EventBus() = default;
    ~EventBus() = default;

    // --- 订阅接口 ---
    // 外界传入 subscriber_id (EntityID) 和一个强类型的回调函数。
    // 回调中应捕获 EntityID 而非 this 指针，以确保生命周期解耦。
    //
    // 用法示例：
    //   int my_id = entity.getId();
    //   bus.subscribe<EntityMovedEvent>(my_id, [my_id](const EntityMovedEvent& e) {
    //       // 通过 my_id 向 World 查询实体是否存活，安全操作
    //   });
    template <typename EventType>
    void subscribe(int subscriber_id, std::function<void(const EventType&)> callback) {
        std::type_index type_idx(typeid(EventType));

        // 将强类型回调包装成接受 std::any 的泛型回调
        EventHandler wrapper = [callback](const std::any& event_data) {
            // 安全地将 std::any 拆包，强转回 EventType，并喂给真正的回调函数
            callback(std::any_cast<const EventType&>(event_data));
        };

        // 以 ID + 包装器的形式挂载到神经网上
        subscribers_[type_idx].push_back({subscriber_id, std::move(wrapper)});
    }

    // --- 注销接口 ---
    // 按 subscriber_id 精确移除指定事件类型下的所有回调。
    // 实体销毁时必须调用此接口，否则将产生悬垂回调。
    template <typename EventType>
    void unsubscribe(int subscriber_id) {
        std::type_index type_idx(typeid(EventType));

        auto it = subscribers_.find(type_idx);
        if (it != subscribers_.end()) {
            auto& subs = it->second;
            subs.erase(
                std::remove_if(subs.begin(), subs.end(),
                    [subscriber_id](const Subscription& s) {
                        return s.subscriber_id == subscriber_id;
                    }),
                subs.end()
            );
        }
    }

    // --- 按 ID 批量注销（跨所有事件类型） ---
    // 当实体被彻底销毁时，一次性清除该 ID 在所有事件类型上的订阅
    void unsubscribeAll(int subscriber_id) {
        for (auto& [type_idx, subs] : subscribers_) {
            subs.erase(
                std::remove_if(subs.begin(), subs.end(),
                    [subscriber_id](const Subscription& s) {
                        return s.subscriber_id == subscriber_id;
                    }),
                subs.end()
            );
        }
    }

    // --- 发布接口 ---
    // 外界扔进来一个具体的事件结构体，例如：publish(EntityMovedEvent{1, 0, 0, 1, 1});
    template <typename EventType>
    void publish(const EventType& event) {
        std::type_index type_idx(typeid(EventType));

        // 如果有人订阅了这个事件，就依次触发他们的回调
        auto it = subscribers_.find(type_idx);
        if (it != subscribers_.end()) {
            for (const auto& sub : it->second) {
                // 将具体事件作为实参传入，C++ 会自动将其隐式转换为 std::any 塞给包装器
                sub.handler(event);
            }
        }
    }
};

} // namespace core
} // namespace fsb
