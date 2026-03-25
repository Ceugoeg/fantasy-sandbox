#pragma once

#include <vector>
#include <memory>
#include <string>
#include "core/EventBus.h"

// 前向声明，避免循环引用
#include <nlohmann/json.hpp>

namespace fsb {
namespace entities { class BaseEntity; }

namespace core {

class CommandQueue;  // 前向声明，解耦头文件依赖

class World {
private:
    // --- 空间属性 ---
    int width_;
    int height_;

    // --- 时间属性 ---
    // 全局单调递增的 Tick ID，作为跨进程时钟同步的唯一序列号。
    // 每次 tick() 自增，注入到每帧 NDJSON 输出中，
    // 供前端按序校验、丢弃乱序包。
    unsigned long long current_tick_;

    // --- 物质与实体 ---
    std::vector<std::shared_ptr<fsb::entities::BaseEntity>> entities_;

    // --- 双层真理网格 ---
    // 1. 生态地貌层 (对应 temp2.json 的 floor 图层)
    // 存储剥离了 Tiled 旋转掩码后的真实图块 ID，用于 AI 判断群落偏好 (如沙漠、草地)
    std::vector<int> terrain_;
    
    // 2. 物理碰撞层 (对应 temp2.json 的 obstacle 图层)
    // 存储绝对的物理阻挡物。若该坐标值大于 0，则绝对不可通行
    std::vector<int> collision_;

    EventBus event_bus_;

public:
    // 创世构造函数：通过读取外部 JSON 地图文件来动态初始化世界边界与网格
    explicit World(const std::string& map_file_path);

    ~World() = default;

    // --- 核心生命周期 ---
    void tick();

    // --- 指令仲裁 ---
    // 在物理 tick 之前统一消费 CommandQueue 中的 Intent，
    // 完成校验与执行。主循环调用顺序：processIntents → tick → export。
    void processIntents(CommandQueue& queue);

    // --- 实体管理 API ---
    void addEntity(std::shared_ptr<fsb::entities::BaseEntity> entity);
    void removeEntity(int entity_id);

    // --- 环境感知 API (AI 与寻路中枢的眼睛) ---
    
    // 获取地貌类型 ID（用于 AI 偏好判定）
    int getTerrainAt(int x, int y) const;
    
    // 绝对物理碰撞检测（用于移动判定与 A* 寻路）
    bool isBlocked(int x, int y) const;

    // 获取局部区域内的实体（用于索敌与感知）
    std::vector<std::shared_ptr<fsb::entities::BaseEntity>> getEntitiesInRange(int x, int y, int radius) const;

    // --- 数据导出 ---
    // 返回 nlohmann::json 对象（包含 tick_id），由调用方负责 dump() + '\n' + fflush。
    // 这一设计将"序列化"与"I/O 冲刷"的职责彻底分离。
    nlohmann::json exportStateAsJson() const;

    // --- 基础 Getter ---
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    unsigned long long getCurrentTick() const { return current_tick_; }
    EventBus& getEventBus() { return event_bus_; }
};

} // namespace core
} // namespace fsb