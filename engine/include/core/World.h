#pragma once

#include <vector>
#include <memory>
#include <string>

// 建立命名空间，防止大型工程中的命名冲突
namespace fsb {      // Fantasy Sandbox

// 前向声明：非模板基类
// 真正的实体类（包含模板策略）会在 entities/ 目录下实现，
// World 只需要知道有这么一个基类即可，以此解耦文件依赖。
namespace entities { class BaseEntity; }

namespace core {

class World {
private:
    // --- 空间属性 ---
    int width_;
    int height_;

    // --- 时间属性 ---
    // 记录世界自创世以来经过了多少个 Tick（帧）
    unsigned long long current_tick_;

    // --- 物质与实体 ---
    // 核心容器：存放世界中所有的活动实体。
    // 使用 std::shared_ptr 自动管理内存，避免裸指针带来的内存泄漏风险。
    // 这是一个多态容器，里面可以装载各种各样被模板实例化出来的具体实体。
    std::vector<std::shared_ptr<fsb::entities::BaseEntity>> entities_;

    // 地形数据：0代表平地，1代表水，2代表高山等。
    // 【原理说明】：在 C++ 中，尽量避免使用 std::vector<std::vector<int>>。
    // 二维 vector 会在堆内存中产生大量碎片，导致 CPU Cache 命中率极低。
    // 使用一维 vector 模拟二维网格（通过 y * width + x 寻址）在内存上是连续的，性能最佳。
    std::vector<int> terrain_;

public:
    // 构造函数：开天辟地，指定沙盘大小
    World(int width, int height);

    // 析构函数：使用 default 即可，智能指针会自动回收 entities_ 中的对象
    ~World() = default;

    // --- 核心生命周期 ---

    // 推动世界时间前进一帧。
    // 这个函数在主循环中被高频调用，它内部会遍历 entities_，调用每个实体的 update()
    void tick();

    // --- 实体管理 API ---

    // 降生：向世界中添加一个实体
    void addEntity(std::shared_ptr<fsb::entities::BaseEntity> entity);

    // 抹杀：移除实体
    void removeEntity(int entity_id);

    // --- 环境感知 API (大模型的“眼睛”) ---

    // 获取某个坐标点的地形类型
    int getTerrainAt(int x, int y) const;

    // 获取某个坐标周围一定半径内的所有实体。
    // 这是极其重要的方法，未来你将提取这里的返回值，拼接成自然语言（Prompt）发给 Node.js 进而传给大模型，
    // 让大模型知道“我周围有一只史莱姆和一片水域”。
    std::vector<std::shared_ptr<fsb::entities::BaseEntity>> getEntitiesInRange(int x, int y, int radius) const;

    // --- 数据导出 (Node.js 通信枢纽) ---

    // 将当前世界的状态（实体坐标、发生变化的区块）序列化。
    // 现阶段可以简单返回一个 JSON 格式的字符串，后续 C++ 会通过某种机制把这个字符串扔给 Node.js。
    std::string exportStateAsJson() const;

    // --- 基础 Getter ---
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    unsigned long long getCurrentTick() const { return current_tick_; }
};

} // namespace core
} // namespace fsb
