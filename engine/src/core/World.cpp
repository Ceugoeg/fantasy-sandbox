#include "core/World.h"
#include "entities/BaseEntity.h"

#include <algorithm>
#include <fstream>
#include <iostream>

// 引入单头文件 JSON 库
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fsb {
namespace core {

World::World(const std::string& map_file_path)
    : current_tick_(0) {
    
    // 1. 读取并解析 Tiled JSON 地图文件
    std::ifstream file(map_file_path);
    if (!file.is_open()) {
        std::cerr << "[World] FATAL ERROR: Cannot open map file: " << map_file_path << std::endl;
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

    std::cerr << "[World] Genesis complete. Map loaded: " << width_ << "x" << height_ << std::endl;
}

void World::tick() {
    current_tick_++;
    
    for (auto& entity : entities_) {
        entity->update(*this);
    }
}

void World::addEntity(std::shared_ptr<fsb::entities::BaseEntity> entity) {
    entities_.push_back(entity);
}

void World::removeEntity(int entity_id) {
    entities_.erase(
        std::remove_if(entities_.begin(), entities_.end(),
            [entity_id](const std::shared_ptr<fsb::entities::BaseEntity>& e) {
                return e->getId() == entity_id;
            }),
        entities_.end()
    );
}

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

std::string World::exportStateAsJson() const {
    // 彻底摒弃危险的字符串手工拼接，采用 nlohmann/json 结构化构建快照
    json j;
    j["tick"] = current_tick_;
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
    
    // dump() 默认生成无缩进、无多余空格的紧凑字符串，极致节省 WebSocket 带宽
    return j.dump(); 
}

} // namespace core
} // namespace fsb