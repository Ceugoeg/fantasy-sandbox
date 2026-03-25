#include "core/World.h"
#include "core/CommandQueue.h"
#include "core/Logger.h"
#include "entities/BaseEntity.h"
#include "entities/EntityFactory.h"

#include <algorithm>
#include <fstream>

// 引入单头文件 JSON 库
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fsb {
namespace core {

// ============================================================================
// 创世构造函数
// ============================================================================
World::World(const std::string& map_file_path)
    : current_tick_(0) {
    
    // 1. 读取并解析 Tiled JSON 地图文件
    std::ifstream file(map_file_path);
    if (!file.is_open()) {
        LOG_ERROR("FATAL: Cannot open map file: %s", map_file_path.c_str());
        exit(EXIT_FAILURE);
    }

    json map_json;
    file >> map_json;

    // 2. 初始化世界尺度
    width_ = map_json["width"];
    height_ = map_json["height"];

    terrain_.resize(width_ * height_, 0);
    collision_.resize(width_ * height_, 0);

    // 3. 遍历解析图层 (Data Sync & Mask Stripping)
    if (map_json.contains("layers")) {
        for (const auto& layer : map_json["layers"]) {
            if (layer["type"] == "tilelayer") {
                std::string layer_name = layer["name"];
                const auto& data = layer["data"];
                
                for (size_t i = 0; i < data.size(); ++i) {
                    unsigned int raw_id = data[i];
                    
                    // 极客位运算：按位与 0x1FFFFFFF，干净利落地剔除高 3 位的旋转/翻转标志
                    int real_id = raw_id & 0x1FFFFFFF;

                    // 将净化后的物理真理写入对应的感知容器
                    if (layer_name == "floor") {
                        terrain_[i] = real_id;
                    } else if (layer_name == "obstacle") {
                        collision_[i] = real_id;
                    }
                }
            }
        }
    }

    LOG_INFO("Genesis complete. Map loaded: %dx%d", width_, height_);
}

// ============================================================================
// 指令仲裁：在 tick 之前统一消费 Intent 队列
// ============================================================================
void World::processIntents(CommandQueue& queue) {
    // 批量消费：一次 tick 窗口内的所有 Intent 全部处理完毕
    while (auto maybe_intent = queue.tryPop()) {
        const Intent& intent = *maybe_intent;

        switch (intent.type) {
            case IntentType::SPAWN: {
                // 实体 ID 的唯一性由单调递增的计数器保证
                static int entity_id_counter = 1000;

                // 将创造实体的重任全权委托给 EntityFactory
                auto entity = fsb::entities::EntityFactory::create(
                    intent.entity_type, entity_id_counter++, 
                    intent.x, intent.y, *this
                );

                addEntity(entity);
                LOG_INFO("Intent executed: SPAWN %s at (%d, %d)", 
                         intent.entity_type.c_str(), intent.x, intent.y);
                break;
            }
            case IntentType::QUIT: {
                LOG_INFO("Intent received: QUIT. Engine will shut down.");
                // QUIT 的传播由 main.cpp 的 is_running 标志处理，
                // 此处仅做日志记录。实际退出逻辑在 main 循环中判断。
                break;
            }
            case IntentType::UNKNOWN: {
                // 已在 CommandQueue::parse 中记录日志，此处静默跳过
                break;
            }
        }
    }
}

// ============================================================================
// 核心 Tick 生命周期
// ============================================================================
void World::tick() {
    current_tick_++;
    
    for (auto& entity : entities_) {
        entity->update(*this);
    }
}

// ============================================================================
// 实体管理
// ============================================================================
void World::addEntity(std::shared_ptr<fsb::entities::BaseEntity> entity) {
    entities_.push_back(entity);
}

void World::removeEntity(int entity_id) {
    // 实体销毁时，先从 EventBus 注销其所有订阅，防止悬垂回调
    event_bus_.unsubscribeAll(entity_id);

    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(),
            [entity_id](const std::shared_ptr<fsb::entities::BaseEntity>& e) {
                return e->getId() == entity_id;
            }),
        entities_.end()
    );
}

// ============================================================================
// 环境感知 API
// ============================================================================
int World::getTerrainAt(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return -1; // 虚空/边界外
    }
    return terrain_[y * width_ + x];
}

bool World::isBlocked(int x, int y) const {
    // 越界视为绝对阻挡
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return true; 
    }
    // 只要 obstacle 图层对应的格子上不是 0（有树木/石头），就不允许通行
    return collision_[y * width_ + x] > 0;
}

std::vector<std::shared_ptr<fsb::entities::BaseEntity>> World::getEntitiesInRange(int x, int y, int radius) const {
    std::vector<std::shared_ptr<fsb::entities::BaseEntity>> result;
    
    for (const auto& entity : entities_) {
        int ex = entity->getX();
        int ey = entity->getY();
        
        int dist_sq = (ex - x) * (ex - x) + (ey - y) * (ey - y);
        if (dist_sq <= radius * radius) {
            result.push_back(entity);
        }
    }
    return result;
}

// ============================================================================
// 状态导出 — 返回 JSON 对象，由调用方负责 dump + flush
// ============================================================================
nlohmann::json World::exportStateAsJson() const {
    // 彻底摒弃危险的字符串手工拼接，采用 nlohmann/json 结构化构建快照
    json j;

    // 注入全局单调递增的 Tick ID，作为跨进程时钟同步的唯一序列号
    j["tick_id"] = current_tick_;
    j["entities"] = json::array();
    
    for (const auto& entity : entities_) {
        j["entities"].push_back({
            {"id", entity->getId()},
            {"type", entity->getTypeName()}, // 暴露 type，供前端多态渲染匹配
            {"x", entity->getX()},
            {"y", entity->getY()},
            {"state", entity->getStateAsString()}
        });
    }
    
    return j;
}

} // namespace core
} // namespace fsb